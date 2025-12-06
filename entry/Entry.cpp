#include "Entry.hpp"
#include "../utils/intrusive_data_structure_utils.hpp"
#include "../utils/log.hpp"

bool are_entries_equal(HNode *node1, HNode *node2) {
    Entry *entry1 = container_of(node1, Entry, node);
    Entry *entry2 = container_of(node2, Entry, node);
    return entry1->key == entry2->key;
}

/* Wrapper function to perform Entry delete as a thread pool task */
void delete_entry_func(void *arg) {
    delete (Entry *) arg;
}

void delete_entry(Entry *entry, MinHeap *ttl_timers, ThreadPool *thread_pool) {
    TTLTimer *timer = &entry->ttl_timer;
    if (timer->expiry_time_ms != 0) {
        ttl_timers->remove(&entry->ttl_timer.node, is_ttl_timer_less);
    }

    if (entry->type == EntryType::SORTED_SET) {
        const uint32_t LARGE_CONTAINER_SIZE = 1000;
        if (entry->zset.length() >= LARGE_CONTAINER_SIZE) {
            log("deleting large sorted set, delegating task to worker threads");
            thread_pool->add_task({ &delete_entry_func, (void *) entry });
            return;
        }
    }
    
    delete entry;
}
