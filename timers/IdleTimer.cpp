#include "IdleTimer.hpp"
#include "../utils/time_utils.hpp"

void IdleTimer::set_expiry(TimerManager *timers) {
    time_t old_expiry_time = expiry_time_ms;
    expiry_time_ms = get_time_ms() + IDLE_TIMEOUT_MS;
    if (old_expiry_time == UNSET) {
        timers->add(this);
    } else {
        timers->update(this);
    }
}

void IdleTimer::clear_expiry(TimerManager *timers) {
    if (!is_expiry_set()) {
        return;
    }
    expiry_time_ms = UNSET;
    timers->remove(this);
}

bool IdleTimer::is_expiry_set() {
    return expiry_time_ms != UNSET;
}
