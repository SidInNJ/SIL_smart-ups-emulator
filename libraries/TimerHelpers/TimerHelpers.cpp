// Handy Timer Functions
// Sid Roberts

#include "TimerHelpers.h" // For Timer_ms and Timer_us. ie. Timer_ms loopTimer;

void Timer_ms::Update( void )
{
    if ((millis()-m_tStart) > m_duration)
    {
        if (m_tActive)
        {
            m_tActive = false;
            m_tCompleted = true;
        }
    }
}

void Timer_ms::Start(uint32_t duration) 
{
    m_tCompleted = false;
    m_tStart = millis();
    if (duration) 
    { 
        m_duration = duration;  
        m_tActive = true;  
    } 
}


void Timer_ms::AddDuration(uint32_t duration, uint32_t maxDuration)  // Start or add to present duration, but don't exceed max into future
{
    uint32_t now = millis();

    if (m_tActive)
    {
        // Already active. Add to duration as long as it won't put us too far into the future
        if (maxDuration && ((now + maxDuration) < (m_tStart + m_duration + duration))) 
        {
            // Duration would have been too long, use the max.
                Start(maxDuration);
        }
        else
        {
            m_duration += duration;
        }
    }
    else
    {
        // Not presently running, just start it.
        Start(duration);
    }
}

// Handy for implementing an every so often tick, just check the return value
bool Timer_ms::StartIfStopped(uint32_t duration)    // Returns true if previous run finished. Starts if not/finished running.
{ 
    Update();   // In case it wasn't called recently.

    bool rc = m_tCompleted;
    if (m_tActive == false)
    {
        m_duration = duration; 
        m_tStart = millis(); 
        m_tActive = true; 
    }

    m_tCompleted = false; 
    return rc;
}

// Did the timer finally time down (complete)? Clear on first test.
bool Timer_ms::isComplete( void )
{
    Update();   // In case it wasn't called recently.
    bool rc = m_tCompleted;
    if (rc)
    {
        m_tActive = false;
    }
    m_tCompleted = false;
    return rc;
}

// If the timer is active, will it not expire in "duration" ms? rc=true if safe to do other stuff
bool Timer_ms::notAlmostDone(uint32_t duration)
{
    bool rc = true;
    if (m_tActive)
    {
        if ((millis() - m_tStart) <= duration)
        {
            rc = false;
        }
    }
    return rc;
}

uint32_t Timer_ms::durLeft(void)
{
    uint32_t rc = 0;

    Update();   // In case it wasn't called recently enough

    if (m_tActive)
    {
        rc = millis() - m_tStart;
    }
    return rc;
}


///////////// Micro-Second Timing /////////// 
 

void Timer_us::Update( void )
{
    if ((micros()-m_tStart) > m_duration)
    {
        if (m_tActive)
        {
            m_tActive = false;
            m_tCompleted = true;
        }
    }
}

// Handy for implementing an every so often tick, just check the return value
bool Timer_us::StartIfStopped(uint32_t duration)    // Returns true if previous run finished. Starts if not/finished running.
{ 
    Update();   // In case it wasn't called recently.

    bool rc = m_tCompleted;
    if (m_tActive == false)
    {
        m_duration = duration; 
        m_tStart = micros(); 
        m_tActive = true; 
    }

    m_tCompleted = false; 
    return rc;
}

// Did the timer finally time down (complete)? Clear on first test.
bool Timer_us::isComplete( void )
{
    Update();   // In case it wasn't called recently.
    bool rc = m_tCompleted;
    if (rc)
    {
        m_tActive = false;
    }
    m_tCompleted = false;
    return rc;
}

// If the timer is active, will it not expire in "duration" ms? rc=true if safe to do other stuff
bool Timer_us::notAlmostDone(uint32_t duration)
{
    bool rc = true;
    if (m_tActive)
    {
        if ((micros() - m_tStart) <= duration)
        {
            rc = false;
        }
    }
    return rc;
}
 
