#pragma once

#include <ctime>

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
         * Sets the expiry of the timer and adds it to the min heap of active TTL timers. If the timer is already in the 
         * heap, updates its position in the heap instead.
         * 
         * @param seconds       The expiry timeout in seconds.
         * @param ttl_timers    Pointer to the TTL timers for entries in the kv store.
         */
        void set_expiry(time_t seconds, MinHeap *ttl_timers);

        /**
         * Clears the expiry for the timer and removes it from the min heap of active TTL timers if set.
         * 
         * @param ttl_timers    Pointer to the TTL timers for entries in the kv store.
         */
        void clear_expiry(MinHeap *ttl_timers);

        /* Checks if the timer's expiry is set */
        bool is_expiry_set();
};
