#include "Entry.hpp"
#include "../utils/intrusive_data_structure_utils.hpp"
#include "../utils/log.hpp"

const uint32_t LARGE_ZSET_SIZE = 1000;

bool are_entries_equal(HNode *node1, HNode *node2) {
    Entry *entry1 = container_of(node1, Entry, node);
    Entry *entry2 = container_of(node2, Entry, node);
    return entry1->key == entry2->key;
}

/* Wrapper function to perform Entry delete as a thread pool task */
void delete_entry_func(void *arg) {
    delete (Entry *) arg;
}

void delete_entry(Entry *entry, TimerManager *timers, ThreadPool *thread_pool) {
    entry->ttl_timer.clear_expiry(timers);

    if (entry->type == EntryType::SORTED_SET) {
        if (entry->zset.length() >= LARGE_ZSET_SIZE) {
            thread_pool->add_task({ &delete_entry_func, (void *) entry });
            return;
        }
    }
    
    delete entry;
}
