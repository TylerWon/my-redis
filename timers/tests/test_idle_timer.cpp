#define TEST_MODE

#include <assert.h>

#include "../IdleTimer.hpp"

void test_set_expiry_new_timer() {
    IdleTimer timer;
    TimerManager timers;
    Queue *idle_timers = timers.get_idle_timers();

    assert(idle_timers->is_empty() == true);
    
    timer.set_expiry(&timers);

    assert(timer.expiry_time_ms > 0);
    assert(idle_timers->is_empty() == false);
    assert(idle_timers->front() == &timer.node);
}

void test_set_expiry_existing_timer() {
    IdleTimer timer;
    TimerManager timers;
    Queue *idle_timers = timers.get_idle_timers();

    assert(idle_timers->is_empty() == true);
    
    timer.set_expiry(&timers);

    assert(timer.expiry_time_ms > 0);
    assert(idle_timers->is_empty() == false);
    assert(idle_timers->front() == &timer.node);
    
    time_t old_expiry_time = timer.expiry_time_ms;
    timer.set_expiry(&timers);

    assert(timer.expiry_time_ms > 0);
    assert(timer.expiry_time_ms >= old_expiry_time);
    assert(idle_timers->is_empty() == false);
    assert(idle_timers->front() == &timer.node);
    idle_timers->remove(&timer.node);
    assert(idle_timers->is_empty() == true);
}

void test_clear_expiry_expiry_not_set() {
    IdleTimer timer;
    TimerManager timers;
    Queue *idle_timers = timers.get_idle_timers();

    assert(timer.expiry_time_ms == IdleTimer::UNSET);
    assert(idle_timers->is_empty() == true);

    timer.clear_expiry(&timers);

    assert(timer.expiry_time_ms == IdleTimer::UNSET);
    assert(idle_timers->is_empty() == true);
}

void test_clear_expiry_expiry_set() {
    IdleTimer timer;
    TimerManager timers;
    Queue *idle_timers = timers.get_idle_timers();

    assert(idle_timers->is_empty() == true);
    
    timer.set_expiry(&timers);

    assert(timer.expiry_time_ms > 0);
    assert(idle_timers->is_empty() == false);
    assert(idle_timers->front() == &timer.node);

    timer.clear_expiry(&timers);

    assert(timer.expiry_time_ms == IdleTimer::UNSET);
    assert(idle_timers->is_empty() == true);
}

void test_is_expiry_set_expiry_not_set() {
    IdleTimer timer;
    assert(timer.is_expiry_set() == false);
}

void test_is_expiry_set_expiry_set() {
    IdleTimer timer;
    TimerManager timers;
    timer.set_expiry(&timers);
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
