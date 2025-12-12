#include "TTLTimer.hpp"
#include "../utils/intrusive_data_structure_utils.hpp"
#include "../utils/time_utils.hpp"

/**
 * Callback which checks if one TTLTimer is less than another in a MinHeap.
 * 
 * @param node1 The MHNode contained by the first TTLTimer.
 * @param node2 The MHNode contained by the second TTLTimer.
 * 
 * @return  True if the first TTLTimer is less than the second TTLTimer.
 *          False otherwise.
 */
bool is_ttl_timer_less(MHNode *node1, MHNode *node2) {
    TTLTimer *timer1 = container_of(node1, TTLTimer, node);
    TTLTimer *timer2 = container_of(node2, TTLTimer, node);
    return timer1->expiry_time_ms < timer2->expiry_time_ms;
}


void TTLTimer::set_expiry(time_t seconds, MinHeap *ttl_timers) {
    time_t old_expiry_time = expiry_time_ms;
    expiry_time_ms = get_time_ms() + seconds * 1000;
    if (old_expiry_time == UNSET) {
        ttl_timers->insert(&node, is_ttl_timer_less);
    } else {
        ttl_timers->update(&node, is_ttl_timer_less);
    }
}

void TTLTimer::clear_expiry(MinHeap *ttl_timers) {
    if (!is_expiry_set()) {
        return;
    }
    expiry_time_ms = UNSET;
    ttl_timers->remove(&node, is_ttl_timer_less);
}

bool TTLTimer::is_expiry_set() {
    return expiry_time_ms != UNSET;
}
