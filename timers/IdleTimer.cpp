#include "IdleTimer.hpp"
#include "../utils/time_utils.hpp"

void IdleTimer::set_expiry(Queue *idle_timers) {
    time_t old_expiry_time = expiry_time_ms;
    expiry_time_ms = get_time_ms() + IDLE_TIMEOUT_MS;
    if (old_expiry_time == UNSET) {
        idle_timers->push(&node);
    } else {
        idle_timers->remove(&node);
        idle_timers->push(&node);
    }
}

void IdleTimer::clear_expiry(Queue *idle_timers) {
    if (!is_expiry_set()) {
        return;
    }
    expiry_time_ms = UNSET;
    idle_timers->remove(&node);
}

bool IdleTimer::is_expiry_set() {
    return expiry_time_ms != UNSET;
}
