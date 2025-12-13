#pragma once

#include <cstdint>
#include <ctime>

#include "TimerManager.hpp"
#include "../queue/Queue.hpp"

/**
 * A timer to track the idleness of a connection.
 * 
 * Once the expiry time is exceeded, the connection associated with the timer has been idle for too long and should be 
 * removed.
 */ 
class IdleTimer {
    private:
        static const uint16_t IDLE_TIMEOUT_MS = 60 * 1000; // 1 minute
        static const int8_t UNSET = -1;

        /* Checks if the timer's expiry is set */
        bool is_expiry_set();
    public:
        time_t expiry_time_ms = UNSET;
        QNode node;

        /**
         * Sets the expiry of the timer and adds it to the timer manager. If the timer is already managed by the timer 
         * manager, tells the manager that the timer's expiry has been updated.
         * 
         * Since idle timers have a fixed timeout value, a timeout parameter is unnecessary.
         * 
         * @param timers    Pointer to the timer manager.
         */
        void set_expiry(TimerManager *timers);

        /**
         * Clears the expiry of the timer and removes it from the timer manager. Only does this if the expiry is set.
         * 
         * @param timers    Pointer to the timer manager.
         */
        void clear_expiry(TimerManager *timers);
};
