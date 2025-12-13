#pragma once

#include <ctime>

#include "TimerManager.hpp"
#include "../min-heap/MinHeap.hpp"

/**
 * A timer to track the TTL (time-to-live) of an entry in the kv store. 
 * 
 * Once the expiry time is exceeded, the entry associated with the timer has expired and should be removed.
 */ 
class TTLTimer {
    private:
        static const int8_t UNSET = -1;
    public:
        time_t expiry_time_ms = UNSET;
        MHNode node;

        /** 
         * Sets the expiry of the timer and adds it to the timer manager. If the timer is already managed by the timer 
         * manager, tells the manager that the timer's expiry has been updated.
         * 
         * @param seconds   The expiry timeout in seconds.
         * @param timers    Pointer to the timer manager.
         */
        void set_expiry(time_t seconds, TimerManager *timers);

        /**
         * Clears the expiry of the timer and removes it from the timer manager. Only does this if the expiry is set.
         * 
         * @param timers    Pointer to the timer manager.
         */
        void clear_expiry(TimerManager *timers);

        /* Checks if the timer's expiry is set */
        bool is_expiry_set();
};
