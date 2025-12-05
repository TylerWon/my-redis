#include "TTLTimer.hpp"
#include "../entry/Entry.hpp"
#include "../utils/intrusive_data_structure_utils.hpp"

bool is_ttl_timer_less(MHNode *node1, MHNode *node2) {
    TTLTimer *timer1 = container_of(node1, TTLTimer, node);
    TTLTimer *timer2 = container_of(node2, TTLTimer, node);
    return timer1->expiry_time_ms < timer2->expiry_time_ms;
}
