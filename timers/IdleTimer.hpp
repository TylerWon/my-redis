#pragma once

#include <cstdint>
#include <ctime>

#include "../queue/Queue.hpp"

/**
 * A timer to track the idleness of a connection. Timers of this type have a fixed timeout value.
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
         * Sets the expiry of the timer and adds it to the queue of active idle timers. If the timer is already in the
         * queue, updates its position in the queue instead.
         * 
         * @param idle_timers   Pointer to the idle timers for active connections.
         */
        void set_expiry(Queue *idle_timers);

        /**
         * Clears the expiry of the timer and removes it from the queue of active idle timers if set.
         * 
         * @param idle_timers   Pointer to the idle timers for active connections.
         */
        void clear_expiry(Queue *idle_timers);
};
