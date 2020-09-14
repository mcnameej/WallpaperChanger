//////////////////////////////////////////////////////////////////////////////
//
//  AppBase.cpp
//
//  CApplicationBase class.
//
//----------------------------------------------------------------------------
//
//  Copyright 2020 by State University of Catatonia and other Contributors
//
//  This file is provided under a "BSD 3-Clause" open source license.
//  The full text of the license is provided in the "LICENSE.txt" file.
//
//  SPDX-License-Identifier: BSD-3-Clause
//
//////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "resource.h"

#include "AppBase.h"

//////////////////////////////////////////////////////////////////////////////
//
//  Global data
//
//////////////////////////////////////////////////////////////////////////////

// WTL global module object.
CAppModule _Module;

// Application object pointer.
CApplicationBase* CApplicationBase::s_pAppObject; 

//////////////////////////////////////////////////////////////////////////////
//
//  CApplicationBase ctor/dtor
//
//////////////////////////////////////////////////////////////////////////////

CApplicationBase::CApplicationBase(
    LPCWSTR pwszAppShortName,
    LPCWSTR pwszVendorShortName
)
    :
    m_hInstance(NULL),
    m_pwszCmdLine(nullptr),
    m_nCmdShow(0),
    m_sAppShortName(pwszAppShortName),
    m_sVendorShortName(pwszVendorShortName),
#if USING_GDIPLUS
    m_gdiplusToken(0),
#endif
    m_AppDataDirInitDone(false),
    m_AppDataDir()
{
    // Only one application object can exist at a time.
    ATLASSERT(s_pAppObject == nullptr);
    s_pAppObject = this;
}

CApplicationBase::~CApplicationBase()
{
    s_pAppObject = nullptr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CApplicationBase::WinMain
//
//////////////////////////////////////////////////////////////////////////////

int
CApplicationBase::WinMain(
    HINSTANCE hInstance,
    LPWSTR /*pwszCmdLine*/,
    int nCmdShow
)
{
    m_hInstance = hInstance;
    m_pwszCmdLine = ::GetCommandLineW();
    m_nCmdShow = nCmdShow;

    int argc = 0;
    WCHAR** argv = ::CommandLineToArgvW(m_pwszCmdLine, &argc);

    int exitCode = ParseCmdLine(argc, argv);

    ::LocalFree(argv);

    if (exitCode == 0)
    {
        exitCode = Initialize();

        if (exitCode == 0)
        {
            exitCode = Run();

            Terminate();
        }
    }

    if (exitCode == EXIT_WITH_SUCCESS)
        exitCode = 0;

    return exitCode;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CApplicationBase::ParseCmdLine
//
//////////////////////////////////////////////////////////////////////////////

int
CApplicationBase::ParseCmdLine(
    int /*argc*/,
    WCHAR** /*argv*/
)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CApplicationBase::Initialize
//
//////////////////////////////////////////////////////////////////////////////

int
CApplicationBase::Initialize()
{
    //
    // COM is the worst possible object system,
    // except for all the others!
    //

    HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    ATLASSERT(SUCCEEDED(hr));

    //
    // Initialize OLE (required for Drag and Drop)
    //

    hr = ::OleInitialize(NULL);
    ATLASSERT(SUCCEEDED(hr));

    //
    // Initialize ATL/WTL
    //

    hr = _Module.Init(NULL, m_hInstance);
    ATLASSERT(SUCCEEDED(hr));

    //
    // Initialize common controls
    //

    AtlInitCommonControls(0xFFFF);

    //
    // Initialize GDI+
    //

#if USING_GDIPLUS
    GP::GdiplusStartupInput gdiplusStartupInput;
    GP::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
#endif

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CApplicationBase::Terminate
//
//////////////////////////////////////////////////////////////////////////////

void
CApplicationBase::Terminate()
{
#if USING_GDIPLUS
    GP::GdiplusShutdown(m_gdiplusToken);
#endif

    _Module.Term();

    ::OleUninitialize();

    ::CoUninitialize();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CApplicationBase::Run
//
//////////////////////////////////////////////////////////////////////////////

int
CApplicationBase::Run()
{
    CMessageLoop theLoop;
    _Module.AddMessageLoop(&theLoop);

    if (!CreateMainWindow())
    {
        FatalErrorMessageBox(L"Main window creation failed!");
        return -1;
    }

    int exitCode = theLoop.Run();

    DeleteMainWindow();

    _Module.RemoveMessageLoop();

    return exitCode;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CApplicationBase::SaveWindowPlacement
//
//////////////////////////////////////////////////////////////////////////////

void
CApplicationBase::SaveWindowPlacement(
    HWND hMainWindow
)
{
    // Get the window location/size.
    WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
    ::GetWindowPlacement(hMainWindow, &placement);

    // Any state other than maximized becomes SW_NORMAL.
    // Don't want to start up minimized on the next run.
    if (placement.showCmd != SW_MAXIMIZE)
        placement.showCmd = SW_NORMAL;

    CRegistryKey appKey = GetAppRegistryKey();
    ATLASSERT(appKey.IsOpen());

    appKey.Write(L"Window.show", placement.showCmd);
    appKey.Write(L"Window.rect", placement.rcNormalPosition);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CApplicationBase::LoadWindowPlacement
//
//////////////////////////////////////////////////////////////////////////////

bool
CApplicationBase::LoadWindowPlacement(
    int& nCmdShow,
    CRect& rcSaved
)
{
    CRegistryKey appKey = GetAppRegistryKey();

    if (!appKey.Read(L"Window.show", nCmdShow) ||
        !appKey.Read(L"Window.rect", rcSaved))
    {
        nCmdShow = m_nCmdShow;
        rcSaved.SetRectEmpty();
        return false;
    }
    else
    {
        return true;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  CApplicationBase::AdjustWindowPlacement
//
//////////////////////////////////////////////////////////////////////////////

CRect*
CApplicationBase::AdjustWindowPlacement(
    int& nCmdShow,
    CRect& rcSaved,
    CRect& rcCreate
)
{
    // Any saved state other than maximized becomes SW_NORMAL.
    if (nCmdShow != SW_MAXIMIZE)
        nCmdShow = SW_NORMAL;

    // Use nCmdShow from application object if minimized or maximized.
    int nAppCmdShow = m_nCmdShow;
    if (nAppCmdShow == SW_MAXIMIZE ||
        nAppCmdShow == SW_MINIMIZE ||
        nAppCmdShow == SW_SHOWMINIMIZED ||
        nAppCmdShow == SW_SHOWMINNOACTIVE ||
        nAppCmdShow == SW_HIDE)
    {
        nCmdShow = nAppCmdShow;
    }

    CRect rcDesktop;
    CRect rcInter;

    // Get desktop rectangle.
    MONITORINFO mi = { sizeof(MONITORINFO) };
    if (::GetMonitorInfoW(::MonitorFromPoint(rcSaved.TopLeft(), MONITOR_DEFAULTTONEAREST), &mi))
        rcDesktop = mi.rcWork;
    else
        ::SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcDesktop, 0);

    rcCreate.SetRectEmpty();

    if (rcInter.IntersectRect(&rcDesktop, &rcSaved))
    {
        // Keep window on the desktop.
        rcSaved = rcInter;
        rcCreate = rcInter;
    }

    if (nCmdShow == SW_MAXIMIZE)
    {
        // Create window so it uses the whole desktop.
        rcCreate = rcDesktop;
    }

    return rcCreate.IsRectEmpty() ? nullptr : &rcCreate;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CApplicationBase::GetAppRegistryKeyPath
//
//////////////////////////////////////////////////////////////////////////////

static void AppendToKeyPath(std::wstring& strKeyPath, const std::wstring& strAddition)
{
    if (!strAddition.empty())
    {
        if (!strKeyPath.empty() && strKeyPath.back() != L'\\')
            strKeyPath.append(L"\\", 1);
        strKeyPath.append(strAddition);
    }
}

static void AppendToKeyPath(std::wstring& strKeyPath, LPCWSTR pwszAddition)
{
    if (pwszAddition && *pwszAddition)
    {
        if (!strKeyPath.empty() && strKeyPath.back() != L'\\')
            strKeyPath.append(L"\\", 1);
        strKeyPath.append(pwszAddition);
    }
}

std::wstring
CApplicationBase::GetAppRegistryKeyPath(
    LPCWSTR pwzSubKey /*= nullptr*/
)
{
    std::wstring strKey;
    strKey.reserve(128);

    AppendToKeyPath(strKey, L"Software");
    AppendToKeyPath(strKey, m_sVendorShortName);
    AppendToKeyPath(strKey, m_sAppShortName);
    AppendToKeyPath(strKey, pwzSubKey);

    return strKey;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CApplicationBase::GetAppDataDirectory
//
//////////////////////////////////////////////////////////////////////////////

fs::path&
CApplicationBase::GetAppDataDirectory()
{
    // The first call to this function will be from WinMain, before
    // any threads are created. No need for thread-safe InitOnce.

    if (!m_AppDataDirInitDone)
    {
        // We only try to initialize once.
        // If it fails, it stays failed.
        m_AppDataDirInitDone = true;

        m_AppDataDir = GetKnownFolderPath(FOLDERID_RoamingAppData);

        if (!m_sVendorShortName.empty())
            m_AppDataDir /= m_sVendorShortName;

        m_AppDataDir /= m_sAppShortName;

        std::error_code ec;
        if (fs::create_directories(m_AppDataDir, ec), ec)  // <- Note use of comma operator!
            m_AppDataDir.clear();
    }

    return m_AppDataDir;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CApplicationBase::GetAppDataFilePath
//
//////////////////////////////////////////////////////////////////////////////

fs::path
CApplicationBase::GetAppDataFilePath(
    ConstWString filename
)
{
    return GetAppDataDirectory() / (LPCWSTR) filename;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CApplicationBase::FatalErrorMessageBox
//
//////////////////////////////////////////////////////////////////////////////

void
CApplicationBase::FatalErrorMessageBox(
    ConstWString errorMessage
)
{
    ::MessageBoxW(NULL, errorMessage, L"Fatal error", MB_ICONEXCLAMATION | MB_OK);
}
