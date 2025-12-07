#pragma once

#include "../buffer/Buffer.hpp"
#include "../sorted-set/SortedSet.hpp"
#include "../min-heap/MinHeap.hpp"
#include "../timers/IdleTimer.hpp"
#include "../timers/TTLTimer.hpp"
#include "../thread-pool/ThreadPool.hpp"

/* Type of Entry */
enum EntryType {
    STR,
    SORTED_SET
};

/**
 * Entry in the kv store.
 * 
 * The value of the Entry is one of str or zset depending on the type.
 */
struct Entry {
    HNode node;
    std::string key;
    // type 
    EntryType type;
    std::string str;
    SortedSet zset;
    // timers
    TTLTimer ttl_timer;
};

/* Simplified version of Entry used for look-ups */
struct LookupEntry {
    HNode node;
    std::string key;
};

extern const uint32_t LARGE_ZSET_SIZE;

/**
 * Callback which checks if two hash map Entries are equal.
 * 
 * @param node1 The HNode contained by the first entry.
 * @param node2 The HNode contained by the second entry.
 * 
 * @return  True if the Entries are equal.
 *          False if not. 
 */
bool are_entries_equal(HNode *node1, HNode *node2);

/**
 * Deletes (deallocates) an Entry.
 * 
 * For sorted set entries with greater th an or equal to LARGE_ZSET_SIZE pairs, the delete will happen asynchronously 
 * using the thread pool workers.
 * 
 * @param entry         Pointer to the Entry to delete.
 * @param ttl_timers    Pointer to the TTL timers for entries in the kv store.
 * @param thread_pool   Pointer to the thread pool.
 */
void delete_entry(Entry *entry, MinHeap *ttl_timers, ThreadPool *thread_pool);
