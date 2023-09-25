// Handy Timer Functions
// Sid Roberts

#ifndef HANDY_TIMER_H
#define HANDY_TIMER_H

#include <Arduino.h>

class Timer_ms {
public: 
    inline Timer_ms(void) { m_tCompleted=false; m_tActive=false; }
    void Start(uint32_t duration);
    void AddDuration(uint32_t duration, uint32_t maxDuration = 0);  // Start or add to present duration, but don't exceed max into future
    inline void Stop(void) { m_duration = 0; m_tActive = false; m_tCompleted = false; }
    bool StartIfStopped(uint32_t duration);
    inline bool isActive( void ) { Update(); return m_tActive; }
    void Update( void );
    bool isComplete( void );

    // If the timer is active, will it not expire in "duration" ms? rc=true if safe to do other stuff
    bool notAlmostDone(uint32_t duration);     
    uint32_t durLeft(void);     

private:
    uint32_t m_tStart;
    uint32_t m_duration;
    bool m_tActive;
    bool m_tCompleted;
};



class Timer_us {
public: 
    inline Timer_us(void) { m_tCompleted=false; m_tActive=false; }
    inline void Start(uint32_t duration) { m_duration = duration; m_tStart = micros(); m_tActive = true; m_tCompleted = false; }
    inline void Stop(void) { m_duration = 0; m_tActive = false; m_tCompleted = false; }
    bool StartIfStopped(uint32_t duration);
    inline bool isActive( void ) { Update(); return m_tActive; }
    void Update( void );
    bool isComplete( void );

    // If the timer is active, will it not expire in "duration" ms? rc=true if safe to do other stuff
    bool notAlmostDone(uint32_t duration);     

private:
    uint32_t m_tStart;
    uint32_t m_duration;
    bool m_tActive;
    bool m_tCompleted;
};


// Prototypes


#endif //end HANDY_TIMER_H



