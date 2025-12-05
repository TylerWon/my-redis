#pragma once

#include <ctime>

#include "../min-heap/components/MHNode.hpp"

/**
 * A timer to track the TTL (time-to-live) of an entry in the kv store. 
 * 
 * Once the expiry time is exceeded, the entry associated with the timer has expired and should be removed.
 */ 
struct TTLTimer {
    time_t expiry_time_ms = 0;
    MHNode node;
};

/**
 * Callback which checks if one TTLTimer is less than another in a MinHeap.
 * 
 * @param node1 The MHNode contained by the first TTLTimer.
 * @param node2 The MHNode contained by the second TTLTimer.
 * 
 * @return  True if the first TTLTimer is less than the second TTLTimer.
 *          False otherwise.
 */
bool is_ttl_timer_less(MHNode *node1, MHNode *node2);
