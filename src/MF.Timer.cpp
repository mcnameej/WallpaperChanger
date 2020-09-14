//////////////////////////////////////////////////////////////////////////////
//
//  MF.Timer.cpp
//
//  CMainFrame countdown timer functions.
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

#include "WallpaperChangerApp.h"

#include "Options.h"
#include "MainFrame.h"

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnCountdownTimer
//
//////////////////////////////////////////////////////////////////////////////

// Handle WM_TMER message for ID_COUNTDOWN_TIMER.
void CMainFrame::OnCountdownTimer(UINT_PTR /*nID*/)
{
    DebugPrintCmdSpew("WM_TIMER: ID_COUNTDOWN_TIMER\n");

    if (!IsPaused())
    {
        // Change wallpaper and restart timer.
        ShowNextOrPrev(ID_WALLPAPER_NEXT);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnUserInterfaceUpdateTimer
//
//////////////////////////////////////////////////////////////////////////////

// Handle WM_TMER message for ID_UI_UPDATE_TIMER.
void CMainFrame::OnUserInterfaceUpdateTimer(UINT_PTR /*nID*/)
{
    DebugPrintCmdSpew("WM_TIMER: ID_UI_UPDATE_TIMER\n");

    // Display the countdown time remaining.
    ShowCountdownTimer();

    // Stop UI timer if window isn't visible or we're paused.
    if (!m_WindowIsVisible || IsPaused())
        StopUserInterfaceUpdateTimer();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::SetCountdownTimerInterval
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::SetCountdownTimerInterval()
{
    // Minimum of one minute, and maximum of 500 hours, between wallpaper changes.
    int changeIntervalInMinutes = std::clamp(GetAppOptions()->m_ChangeInterval, 1, 500*60);

#if 1
    time_t changeIntervalInSeconds = changeIntervalInMinutes * 60;
#else
    // Treat change interval as seconds instead of minutes.
    // 60X faster changes for debugging.
    time_t changeIntervalInSeconds = changeIntervalInMinutes;
#endif

    m_CountdownTimer.SetDuration(changeIntervalInSeconds);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::BeginCountdown
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::BeginCountdown()
{
    // Stop the current countdown (if any).
    m_CountdownTimer.Stop();

    // Exit the paused state (if we were paused).
    ExitPausedState();

    // Update timer interval in case our options have changed.
    SetCountdownTimerInterval();

    DebugPrintCmdSpew("BeginCountdown: %d seconds\n", (int) m_CountdownTimer.GetDuration());

    // Start countdown timer.
    m_CountdownTimer.Start();

    // Show countdown in user interface.
    ShowCountdownTimer();

    // Start timer to update countdown in user interface.
    StartUserInterfaceUpdateTimer();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::ShowCountdownTimer
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::ShowCountdownTimer()
{
    if (!m_CountdownTimer.IsStarted())
    {
        // UISetText(ID_DEFAULT_PANE, L"Countdown paused");
        m_Toolbar_CountdownTimer.SetWindowTextW(L"PAUSED");
    }
    else
    {
        int hoursRemaining, minutesRemaining, secondsRemaining;
        WCHAR wszCountdownTimer[64];

        m_CountdownTimer.GetTimeRemaining(hoursRemaining,
                                          minutesRemaining,
                                          secondsRemaining);

        // StringCbPrintfW(wszCountdownTimer,
        //                 sizeof(wszCountdownTimer),
        //                 L"Countdown: %02d:%02d:%02d",
        //                 hoursRemaining,
        //                 minutesRemaining,
        //                 secondsRemaining);
        // 
        // if (IsPaused())
        //     StringCbCatW(wszCountdownTimer, sizeof(wszCountdownTimer), L" (paused)");
        // 
        // UISetText(ID_DEFAULT_PANE, wszCountdownTimer);

        StringCbPrintfW(wszCountdownTimer,
                        sizeof(wszCountdownTimer),
                        L"%02d:%02d:%02d",
                        hoursRemaining,
                        minutesRemaining,
                        secondsRemaining);

        m_Toolbar_CountdownTimer.SetWindowTextW(wszCountdownTimer);
    }

    // UIUpdateStatusBar();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::PauseTimedChanges
//  CMainFrame::ResumeTimedChanges
//  CMainFrame::ExitPausedState
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::PauseTimedChanges()
{
    if (!IsPaused())
    {
        m_CountdownTimer.Pause();

        GetAppOptions()->m_Paused = true;
        GetAppOptions()->SaveToRegistry();

        UpdateForPausedState();
    }
}

void CMainFrame::ResumeTimedChanges()
{
    if (IsPaused())
    {
        ExitPausedState();

        if (m_CountdownTimer.IsPaused())
            m_CountdownTimer.Resume();
        else
            BeginCountdown();
    }
}

void CMainFrame::ExitPausedState()
{
    if (IsPaused())
    {
        GetAppOptions()->m_Paused = false;
        GetAppOptions()->SaveToRegistry();

        UpdateForPausedState();
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::UpdateForPausedState
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateForPausedState()
{
    DebugPrintCmdSpew("UpdateForPausedState: IsPaused = %s\n", IsPaused() ? "true" : "false");

    UISetCheck(ID_WALLPAPER_PAUSE, IsPaused());

    // Show the current countdown.
    // Do this even if paused, and even if window is hidden.
    ShowCountdownTimer();

    // Start UI Update timer.
    StartUserInterfaceUpdateTimer();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::StartUserInterfaceUpdateTimer
//  CMainFrame::StopUserInterfaceUpdateTimer
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::StartUserInterfaceUpdateTimer()
{
    // Start timer if the window is visible and changes aren't paused.
    //
    // If timer was already started, resets it to use current time as the
    // baseline.  If called immediately before/after staring the countdown
    // timer, the net effect is to synchronize the timers, resulting in a
    // smoother UI.

    if (m_WindowIsVisible && !IsPaused())
    {
        if (!m_UserInterfaceUpdateTimerStarted)
        {
            DebugPrintCmdSpew("Start ID_UI_UPDATE_TIMER\n");
            m_UserInterfaceUpdateTimerStarted = true;
        }
        SetTimer(ID_UI_UPDATE_TIMER, 1000, NULL);
    }
}

void CMainFrame::StopUserInterfaceUpdateTimer()
{
    if (m_UserInterfaceUpdateTimerStarted)
    {
        DebugPrintCmdSpew("Stop ID_UI_UPDATE_TIMER\n");
        m_UserInterfaceUpdateTimerStarted = false;
        KillTimer(ID_UI_UPDATE_TIMER);
    }
}
