#pragma once

#include <cstdint>

#include "HNode.hpp"

/* A fixed-size chaining hashtable using linked lists */
class HTable {
    private:
        uint64_t mask; // num_slots - 1, used to perform modulo without division
    public:
        HNode **table;    // array of linked lists
        uint64_t num_slots;  
        uint64_t num_keys;
    
        /* Initializes a HTable with n slots. */
        HTable(uint64_t n);

        ~HTable();

        /**
         * Inserts the node into the HTable.
         * 
         * @param node  The node to insert.
         */
        void insert(HNode *node);

        /**
         * Searches for the key in the HTable and returns its node if found.
         * 
         * @param key   A HNode containing the key to search for.
         * @param eq    A function that checks for the equality of two nodes.
         * 
         * @return  If the node is found, the address of the pointer that points to the node (i.e. address of 'next' in 
         *          the node's parent or the slot in the table if the node is the head of the slot's linked list).
         *          NULL if the node is not found.
         */
        HNode **lookup(HNode *key, bool (*eq)(HNode *, HNode *));

        /**
         * Detaches the node pointed to by *from from the linked list it is a part of.
         * 
         * @param from  The address of the pointer that points to the node. 
         * 
         * @return  A pointer to the detached node.
         */
        HNode *detach(HNode **from);

        /**
         * Executes the provided callback function on each of the nodes in the HTable.
         * 
         * @param cb        The callback function.
         * @param cb_arg    An argument for the callback. Void pointer type allows flexibility in argument type for 
         *                  different callbacks.
         */
        void for_each(void (*cb)(HNode *, void *), void *cb_arg);
};