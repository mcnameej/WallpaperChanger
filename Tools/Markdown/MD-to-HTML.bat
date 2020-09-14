@echo off
setlocal

set RC=0
set TEMPFILE="%TEMP%\MD-to-HTML-%RANDOM%.html"

if "%1" == "--help" goto usage
if "%1" == "/?" goto usage
if "%1" == "" goto usage
if "%2" == "" goto usage
if not "%3" == "" goto usage

set SRC="%1"
set DST="%2"

rem --------------------------------------------------------------------------

"%~dp0\cmark.exe" -t html --smart %SRC% >%TEMPFILE%
set RC=%ERRORLEVEL%
if not "%RC%" == "0" goto done

type "%~dp0\header.html" >%DST%
type %TEMPFILE% >>%DST%
type "%~dp0\footer.html" >>%DST%

goto done

rem --------------------------------------------------------------------------

:usage

echo usage: %0 ^<MD-file^> ^<HTML-file^>
set RC=1
goto done

rem --------------------------------------------------------------------------

:done

if exist %TEMPFILE% del %TEMPFILE% >NUL 2>&1

cmd /c exit %RC%

endlocal

cmd /c exit %ERRORLEVEL%
