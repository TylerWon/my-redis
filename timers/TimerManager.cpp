#include "IdleTimer.hpp"
#include "TimerManager.hpp"
#include "TTLTimer.hpp"
#include "../conn/Conn.hpp"
#include "../entry/Entry.hpp"
#include "../utils/intrusive_data_structure_utils.hpp"
#include "../utils/log.hpp"
#include "../utils/time_utils.hpp"

int32_t TimerManager::get_time_until_expiry() {
    time_t next_expiry_ms = -1;
    time_t now_ms = get_time_ms();

    if (!idle_timers.is_empty()) {
        QNode *node = idle_timers.front();
        IdleTimer *timer = container_of(node, IdleTimer, node);
        next_expiry_ms = timer->expiry_time_ms;
    }

    if (!ttl_timers.is_empty()) {
        MHNode *node = ttl_timers.min();
        TTLTimer *timer = container_of(node, TTLTimer, node);
        if (next_expiry_ms == -1 || timer->expiry_time_ms < next_expiry_ms) {
            next_expiry_ms = timer->expiry_time_ms;
        }
    }

    if (next_expiry_ms == -1) {
        return -1;
    } else if (now_ms >= next_expiry_ms) {
        return 0;
    }

    return next_expiry_ms - now_ms;
}

void TimerManager::process_timers(HMap &kv_store, std::vector<Conn *> &fd_to_conn, ThreadPool &thread_pool) {
    time_t now_ms = get_time_ms();
    while (!idle_timers.is_empty()) {
        QNode *node = idle_timers.front();
        IdleTimer *timer = container_of(node, IdleTimer, node);
        if (timer->expiry_time_ms > now_ms) {
            break;
        }
        Conn *conn = container_of(timer, Conn, idle_timer);
        log("connection %d exceeded idle timeout", conn->fd);
        conn->handle_close(fd_to_conn, this);
    }

    uint32_t count = 0;
    while (!ttl_timers.is_empty() && count < MAX_TTL_EXPIRATIONS) {
        MHNode *node = ttl_timers.min();
        TTLTimer *timer = container_of(node, TTLTimer, node);
        if (timer->expiry_time_ms > now_ms) {
            break;
        }
        Entry *entry = container_of(timer, Entry, ttl_timer);
        log("key '%s' expired", entry->key.data());
        kv_store.remove(&entry->node, are_entries_equal);
        delete_entry(entry, this, &thread_pool);
        count++;
    }
}

void TimerManager::add(IdleTimer *timer) {
    idle_timers.push(&timer->node);
}

void TimerManager::update(IdleTimer *timer) {
    idle_timers.remove(&timer->node);
    idle_timers.push(&timer->node);
}

void TimerManager::remove(IdleTimer *timer) {
    idle_timers.remove(&timer->node);
}

void TimerManager::add(TTLTimer *timer) {
    ttl_timers.insert(&timer->node, is_ttl_timer_less);
}

void TimerManager::update(TTLTimer *timer) {
    ttl_timers.update(&timer->node, is_ttl_timer_less);
}

void TimerManager::remove(TTLTimer *timer) {
    ttl_timers.remove(&timer->node, is_ttl_timer_less);
}
