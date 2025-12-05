#pragma once

#include <cstdint>

#include "../../hashmap/components/HNode.hpp"
#include "../../avl-tree/components/AVLNode.hpp"

/* Pair stored in a SortedSet */
struct SPair {
    HNode map_node;
    AVLNode tree_node;

    double score = 0;
    uint32_t len = 0;
    char name[0]; // flexible array
};

/**
 * Simplified version of SPair to use for look-ups in the AVLTree.
 */
struct AVLLookupPair {
    AVLNode node;
    double score = 0;
    const char *name = NULL;
    uint32_t len = 0;
};

/**
 * Simplified version of SPair to use for look-ups in the HMap.
 */
struct HLookupPair {
    HNode node;
    const char *name = NULL;
    uint32_t len = 0;
};

/**
 * Dynamically allocates an SPair.
 * 
 * Can't use "new" because SPair contains a flexible array which C++ does not know how to allocate. 
 * 
 * @param name  Byte array that store the name.
 * @param len   Length of the name.
 * @param score The score.
 * 
 * @return  Pointer to the SPair.
 */
SPair *spair_new(const char *name, uint32_t len, double score);

/**
 * Deletes a dynamically allocated SPair.
 * 
 * @param pair  Pointer to the SPair to delete.
 */
void spair_del(SPair *pair);
