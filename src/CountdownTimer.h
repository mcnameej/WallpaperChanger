//////////////////////////////////////////////////////////////////////////////
//
//  CountdownTimer.h
//
//  CCountdownTimer class.
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

#pragma once

// Enable or disable debug spew from CCountdownTimer.
#define ENABLE_COUNTDOWN_TIMER_SPEW 0

#if ENABLE_COUNTDOWN_TIMER_SPEW
  #define CountdownTimerDebugPrint(format, ...) DebugPrint(format, ##__VA_ARGS__)
#else
  #define CountdownTimerDebugPrint(format, ...) do {} while (0)
#endif

// Countdown timer.
class CCountdownTimer
{
    public:

        // Construct CCountdownTimer object.
        CCountdownTimer()
            :
            m_Started(false),
            m_Paused(false),
            m_Duration(0),
            m_EndTime(0),
            m_TimeRemainingWhenPaused(0),
            m_WindowsTimerStared(false),
            m_hWindow(NULL),
            m_TimerID(0)
        {
        }

        ~CCountdownTimer()
        {
            Stop();
        }

        // No copy ctor.
        CCountdownTimer(const CCountdownTimer&) = delete;

        // No copy assignment.
        CCountdownTimer& operator=(const CCountdownTimer&) = delete;

    public:

        // Initialize countdown timer.
        void
        Initialize(
            HWND hWindow,
            UINT TimerID
        )
        {
            ATLASSERT(hWindow != NULL && TimerID != 0);
            m_hWindow = hWindow;
            m_TimerID = TimerID;
        }

        // Specify duration and start countdown.
        void
        Start(
            time_t duration
        )
        {
            SetDuration(duration);
            Start();
        }

        // Start countdown.
        void
        Start()
        {
            // If a countdown is already started, calling Start() again
            // will start a new countdown with the standard duration.
            // Also, Start() always clears the paused state.

            CountdownTimerDebugPrint("CCountdownTimer::Start\n");

            ATLASSERT(m_Duration != 0);
            m_Started = true;
            m_Paused = false;
            m_EndTime = Now() + m_Duration;
            m_TimeRemainingWhenPaused = 0;
            StartWindowsTimer();
        }

        // Start new countdown. Has no effect if countdown
        // was never started, or if countdown is paused.
        bool
        StartAgain()
        {
            CountdownTimerDebugPrint("CCountdownTimer::StartAgain\n");

            if (m_Started && !m_Paused)
            {
                Start();
                return true;
            }
            else
            {
                return false;
            }
        }

        // Stop countdown.
        void
        Stop()
        {
            CountdownTimerDebugPrint("CCountdownTimer::Stop\n");

            m_Started = false;
            m_Paused = false;
            m_EndTime = 0;
            m_TimeRemainingWhenPaused = 0;
            StopWindowsTimer();
        }

        // Pause countdown.
        // No effect if countdown wasn't started.
        void
        Pause()
        {
            if (m_Started && !m_Paused)
            {
                // IMPORTANT: Must call GetTimeRemaining() before setting m_Paused.
                m_TimeRemainingWhenPaused = GetTimeRemaining();
                m_Paused = true;
                CountdownTimerDebugPrint("CCountdownTimer::Pause: %d seconds remaining\n", (int) m_TimeRemainingWhenPaused);
                StopWindowsTimer();
            }
        }

        // Resume a paused countdown.
        // No effect if contdown wasn't paused.
        void
        Resume()
        {
            if (m_Paused)
            {
                CountdownTimerDebugPrint("CCountdownTimer::Resume\n");
                m_Paused = false;
                m_EndTime = Now() + m_TimeRemainingWhenPaused;
                m_TimeRemainingWhenPaused = 0;
                StartWindowsTimer();
            }
        }

        // Check if countdown has reached zero.
        bool
        ReachedZero()
        {
            // Countdown can't "reach zero" if not started or if paused.
            if (!m_Started || m_Paused)
                return false;

            return GetTimeRemaining() == 0;
        }

        // Get time remaining in seconds.
        // Guaranteed to be non-negative value.
        time_t
        GetTimeRemaining()
        {
            if (!m_Started)
            {
                // Return 0 if timer isn't started.
                return 0;
            }
            else if (m_Paused)
            {
                // Return time remaining when timer was paused.
                return m_TimeRemainingWhenPaused;
            }
            else
            {
                // Return current time remaining.
                return std::max(m_EndTime - Now(), (time_t) 0);
            }
        }

        // Get time remaining in hours/minutes/seconds.
        time_t
        GetTimeRemaining(
            int& hoursRemaining,
            int& minutesRemaining,
            int& secondsRemaining
        )
        {
            time_t timeRemaining = GetTimeRemaining();

            hoursRemaining   = (int)(timeRemaining / (60*60));
            minutesRemaining = (int)((timeRemaining % (60*60)) / 60);
            secondsRemaining = (int)(timeRemaining % 60);

            return timeRemaining;
        }

        // Get time remaining in milliseconds (e.g. for Windows timer).
        UINT
        GetTimeRemainingMS()
        {
            return (UINT) GetTimeRemaining() * 1000U;
        }

    public:

        bool
        IsStarted()
        {
            return m_Started;
        }

        bool
        IsPaused()
        {
            return m_Paused;
        }

        time_t
        GetDuration()
        {
            return m_Duration;
        }

        void
        SetDuration(
            time_t duration
        )
        {
            ATLASSERT(duration > 0);
            m_Duration = duration;
        }

    private:

        // Get the current time.
        FORCEINLINE
        time_t
        Now()
        {
            return (time_t) (::GetTickCount64() / 1000ULL);
        }

        // Start Win32 timer.
        void
        StartWindowsTimer()
        {
            // NOTE: StartWindowsTimer will either start the timer,
            // or update existing timer to use new timer period.

            if (m_hWindow != NULL && m_TimerID != 0)
            {
                m_WindowsTimerStared = true;

                UINT timerPeriod = std::max(GetTimeRemainingMS(), (UINT)USER_TIMER_MINIMUM);

                ::SetTimer(m_hWindow,
                           m_TimerID,
                           timerPeriod,
                           nullptr);

                CountdownTimerDebugPrint("CCountdownTimer::StartWindowsTimer: %u seconds\n", timerPeriod / 1000U);
            }
        }

        // Stop Win32 timer.
        void
        StopWindowsTimer()
        {
            if (m_WindowsTimerStared)
            {
                m_WindowsTimerStared = false;

                ::KillTimer(m_hWindow, m_TimerID);

                CountdownTimerDebugPrint("CCountdownTimer::StopWindowsTimer\n");
            }
        }

    private:

        bool m_Started;
        bool m_Paused;
        time_t m_Duration;
        time_t m_EndTime;
        time_t m_TimeRemainingWhenPaused;

        bool m_WindowsTimerStared;
        HWND m_hWindow;
        UINT m_TimerID;
};
