#include "TTLTimer.hpp"
#include "../utils/intrusive_data_structure_utils.hpp"
#include "../utils/time_utils.hpp"

void TTLTimer::set_expiry(time_t seconds, TimerManager *timers) {
    time_t old_expiry_time = expiry_time_ms;
    expiry_time_ms = get_time_ms() + seconds * 1000;
    if (old_expiry_time == UNSET) {
        timers->add(this);
    } else {
        timers->update(this);
    }
}

void TTLTimer::clear_expiry(TimerManager *timers) {
    if (!is_expiry_set()) {
        return;
    }
    expiry_time_ms = UNSET;
    timers->remove(this);
}

bool TTLTimer::is_expiry_set() {
    return expiry_time_ms != UNSET;
}
