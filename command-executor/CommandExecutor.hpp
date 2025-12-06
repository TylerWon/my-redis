#pragma once

#include <string>

#include "../entry/Entry.hpp"
#include "../hashmap/HMap.hpp"
#include "../min-heap/MinHeap.hpp"
#include "../response/Response.hpp"
#include "../request/Request.hpp"

/* Executes a Redis command */
class CommandExecutor {
    private:
        HMap *kv_store;
        MinHeap *ttl_timers;
        ThreadPool *thread_pool;
        
        /**
         * Searches for the Entry with the given key in the kv store.
         * 
         * @param key   The key of the entry to look for.
         * 
         * @return  Pointer to the Entry if found.
         *          NULL otherwise.
         */
        Entry *lookup_entry(const std::string &key);

        /**
         * Gets the entry for the provided key in the kv store.
         * 
         * If the key does not exist the special value nil is returned. 
         * An error is returned if the value stored at key is not a string.
         * 
         * @param key   The key to obtain.
         * 
         * @return  One of the following:
         *          - StrResponse: the value of the key.
         *          - NilResponse: if the key does not exist.
         *          - ErrResponse: if the value stored at the key is not a string.
         */
        Response *do_get(const std::string &key);

        /**
         * Sets the value of the provided key in the kv store. 
         * 
         * If key already exists, updates its value (regardless of type) and clears its TTL (if set).
         * 
         * @param key   The key to set.
         * @param value The value for the key.
         * 
         * @return  StrResponse ("OK"): the key was set.
         */
        Response *do_set(const std::string &key, const std::string &value);

        /**
         * Deletes the entry for the provided key in the kv store.
         * 
         * @param key   The key to delete.
         * 
         * @return  IntResponse: the number of keys removed.
         */
        Response *do_del(const std::string &key);

        /**
         * Gets all keys in the kv store.
         * 
         * @return ArrResponse: a list of keys.
         */
        Response *do_keys();

        /**
         * Adds a pair to the sorted set stored at the given key.
         *
         * If a pair with the given name already exists in the sorted set, its score is updated.
         * If key does not exist, a new sorted set with the specified pair is created.
         * If the key exists but does not hold a sorted set, an error is returned.
         *
         * @param key   The key of the sorted set.
         * @param score The pair's score.
         * @param name  The pair's name.
         *
         * @return  One of the following:
         *          - IntResponse: the number of new or updated pairs.
         *          - ErrResponse: the key does not hold a sorted set.
         */
        Response *do_zadd(const std::string &key, double score, const std::string &name);

        /**
         * Gets the score of name in the sorted set stored at key.
         * 
         * If the key does not exist, the key does not hold a sorted set, or the name is not in the sorted set, a nil
         * is returned.
         * 
         * @param key   The key of the sorted set.
         * @param name  The name of the score to get.
         * 
         * @return  One of the following:
         *          - StrResponse: the score of the name.
         *          - NilResponse: if name does not exist in the sorted set, or the key does not exist.
         */
        Response *do_zscore(const std::string &key, const std::string &name);

        /**
         * Removes name from the sorted set stored at key.
         * 
         * If the key exists but does not hold a sorted set, an error is returned.
         * 
         * @param key   The key of the sorted set.
         * @param name  The name of the pair to remove.
         * 
         * @return  One of the following:
         *          - IntResponse: the number of pairs removed from the sorted set.
         *          - ErrResponse: the key does not hold a sorted set.
         */
        Response *do_zrem(const std::string &key, const std::string &name);

        /**
         * Finds all pairs in the sorted set stored at key greater than or equal to the given pair.
         * 
         * If the key exists but does not hold a sorted set, an error is returned.
         * 
         * @param key       The key of the sorted set.
         * @param score     The pair's score.
         * @param name      The pair's name.
         * @param offset    The number of pairs to exclude from the beginning of the result.
         * @param limit     The maximum number of pairs to return. 0 means no limit.
         * 
         * @return  One of the following:
         *          - ArrResponse: the pairs greater than or equal to the given pair.
         *          - ErrResponse: the key does not hold a sorted set.
         */
        Response *do_zquery(const std::string &key, double score, const std::string &name, uint64_t offset, uint64_t limit);

        /**
         * Gets the rank (position in sorted order) of name in the sorted set stored at key. The rank is 0-based, so the
         * lowest pair is rank 0.
         * 
         * If the key does not exist, the key does not hold a sorted set, or the name is not in the sorted set, a nil
         * is returned.
         * 
         * @param key   The key of the sorted set.
         * @param name  The name of the pair to rank.
         * 
         * @return  One of the following:
         *          - NilResponse: if the key does not exist or the pair does not exist in the sorted set.
         *          - IntResponse: the rank of the pair.
         */
        Response *do_zrank(const std::string &key, const std::string &name);

        /**
         * Sets a timeout on the given key. After the timeout has expired, the key will be deleted.
         * 
         * The timeout will be cleared by commands that delete or overwrite the contents of the key. This includes the 
         * del and set commands.
         * 
         * @param key       The key to set the timeout on.
         * @param seconds   The timeout in seconds.
         * 
         * @return  One of the following:
         *          - IntReponse: 1 if the timeout was set.
         *          - IntResponse: 0 if timeout was not set.
         */
        Response *do_expire(const std::string &key, time_t seconds);

        /**
         * Gets the remaining time-to-live of the given key.
         * 
         * @param key   The key to get the TTL for.
         * 
         * @return  One of the following:
         *          - IntResponse: TTL in seconds.
         *          - IntResponse: -1 if the key exists but has no associated expiration.
         *          - IntResponse: -2 if the key does not exist.
         */
        Response *do_ttl(const std::string &key);

        /**
         * Removes the existing timeout on the given key.
         * 
         * @param key   The key to remove the timeout for.
         * 
         * @return  One of the following:
         *          - IntResponse: 0 if the key does not exist or does not have an associated timeout.
         *          - IntResponse: 1 if the timeout has been removed.
         */
        Response *do_persist(const std::string &key);
    public:
        /* Initializes a CommandExecutor, storing references to the kv store, TTL timers, and thread pool */
        CommandExecutor(HMap *kv_store, MinHeap *ttl_timers, ThreadPool *thread_pool) : kv_store(kv_store), ttl_timers(ttl_timers), thread_pool(thread_pool) {};

        /**
         * Executes the given command.
         * 
         * The following commands are supported:
         * 1. get <key>
         * 2. set <key> <value>
         * 3. del <key>
         * 4. keys
         * 5. zadd <key> <score> <name>
         * 6. zscore <key> <name>
         * 7. zrem <key> <name>
         * 8. zquery <key> <score> <name> <offset> <limit>
         * 9. zrank <key> <name>
         * 10. expire <key> <seconds>
         * 11. ttl <key>
         * 12. persist <key>
         * 
         * @param command   The command to execute, broken up into its individual strings.
         * 
         * @return  Pointer to the Response for executing the command.
         */
        Response *execute(const std::vector<std::string> &command);
};
