#pragma once

#include <string>
#include <cstdint>

#include "components/HTable.hpp"
#include "components/HNode.hpp"

/* Dynamic hashtable using progressive rehashing */
class HMap {
    private:
        HTable *newer;
        HTable *older;
        uint64_t migrate_pos; // last slot in older that keys were migrated from during progressive rehashing
        uint32_t max_load_factor = 8;
        uint32_t num_keys_to_rehash = 128;
    public: 
        /* Initializes a HMap with 8 slots. */
        HMap();

        ~HMap();

        /**
         * Inserts the node into the HMap.
         * 
         * Doubles HMap size if load factor exceeds MAX_LOAD_FACTOR after insert, but does not rehash all keys 
         * immediately. Instead, rehashing occurs progressively; a constant number of keys moved from old table to new 
         * table every time an insert, lookup, or remove is performed until the old table is empty. 
         * 
         * @param node  The node to insert.
         */
        void insert(HNode *node);

        /** 
         * Searches for a node in the HMap with the given key. 
         * 
         * @param key   A HNode whose container stores the key.
         * @param eq    A function that checks for the equality of two nodes.
         * 
         * @return  A pointer to the node if found.
         *          NULL if the node is not found.
         */
        HNode *lookup(HNode *key, bool (*eq)(HNode *, HNode *));

        /**
         * Remove the node in the HMap with the given key. 
         * 
         * @param key   A HNode whose container stores the key.
         * @param eq    A function that checks for the equality of two nodes.
         * 
         * @return  The node that was removed.
         *          NULL if the key does not exist in the HMap.
         */
        HNode *remove(HNode *key, bool (*eq)(HNode *, HNode *));

        /**
         * Executes the provided callback function on each of the nodes in the HMap.
         * 
         * @param cb        The callback function.
         * @param cb_arg    An argument for the callback. Void pointer type allows flexibility in argument type for 
         *                  different callbacks.
         */
        void for_each(void (*cb)(HNode *, void *), void *cb_arg);

        /* Returns the number of keys in the HMap */
        uint32_t length();

        /* Set the max load factor */
        void set_max_load_factor(uint32_t max_load_factor);

        /* Set the number of keys to rehash */
        void set_num_keys_to_rehash(uint32_t num_keys_to_rehash);
    private:
        /** 
         * Rehashes a constant number of keys from the old hashtable to the new one. 
         */
        void rehash_keys();

        /**
         * Moves the newer hashtable to older and allocates a bigger hashtable (2x) for newer.
         */
        void resize();

    #ifdef TEST_MODE
    public:      
        HTable *get_newer() { return newer; }

        HTable *get_older() { return older; }

        uint64_t get_migrate_pos() { return migrate_pos; }
    #endif
};
