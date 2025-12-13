#define TEST_MODE

#include <assert.h>

#include "../TTLTimer.hpp"

void test_set_expiry_new_timer() {
    TTLTimer timer;
    TimerManager timers;
    MinHeap *ttl_timers = timers.get_ttl_timers();

    assert(ttl_timers->is_empty() == true);
    
    timer.set_expiry(100, &timers);

    assert(timer.expiry_time_ms > 0);
    assert(ttl_timers->is_empty() == false);
    assert(ttl_timers->min() == &timer.node);
}

void test_set_expiry_existing_timer() {
    TTLTimer timer;
    TimerManager timers;
    MinHeap *ttl_timers = timers.get_ttl_timers();

    assert(ttl_timers->is_empty() == true);

    timer.set_expiry(100, &timers);

    assert(timer.expiry_time_ms > 0);
    assert(ttl_timers->is_empty() == false);
    assert(ttl_timers->min() == &timer.node);

    time_t old_expiry_time = timer.expiry_time_ms;
    timer.set_expiry(100, &timers);

    assert(timer.expiry_time_ms > 0);
    assert(timer.expiry_time_ms >= old_expiry_time);
    assert(ttl_timers->is_empty() == false);
    assert(ttl_timers->min() == &timer.node);
    ttl_timers->remove(&timer.node, is_ttl_timer_less);
    assert(ttl_timers->is_empty() == true);
}

void test_clear_expiry_expiry_not_set() {
    TTLTimer timer;
    TimerManager timers;
    MinHeap *ttl_timers = timers.get_ttl_timers();

    assert(timer.expiry_time_ms == TTLTimer::UNSET);
    assert(ttl_timers->is_empty() == true);

    timer.clear_expiry(&timers);

    assert(timer.expiry_time_ms == TTLTimer::UNSET);
    assert(ttl_timers->is_empty() == true);
}

void test_clear_expiry_expiry_set() {
    TTLTimer timer;
    TimerManager timers;
    MinHeap *ttl_timers = timers.get_ttl_timers();

    assert(ttl_timers->is_empty() == true);

    timer.set_expiry(100, &timers);

    assert(timer.expiry_time_ms > 0);
    assert(ttl_timers->is_empty() == false);
    assert(ttl_timers->min() == &timer.node);

    timer.clear_expiry(&timers);

    assert(timer.expiry_time_ms == TTLTimer::UNSET);
    assert(ttl_timers->is_empty() == true);
}

void test_is_expiry_set_expiry_not_set() {
    TTLTimer timer;
    assert(timer.is_expiry_set() == false);
}

void test_is_expiry_set_expiry_set() {
    TTLTimer timer;
    TimerManager timers;
    timer.set_expiry(100, &timers);
    assert(timer.is_expiry_set() == true);
}

int main() {
    test_set_expiry_new_timer();
    test_set_expiry_existing_timer();

    test_clear_expiry_expiry_not_set();
    test_clear_expiry_expiry_set();

    test_is_expiry_set_expiry_not_set();
    test_is_expiry_set_expiry_set();
    
    return 0;
}