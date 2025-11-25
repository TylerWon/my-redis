#include <cstring>

#include "SortedSet.hpp"
#include "../utils/intrusive_data_structure_utils.hpp"
#include "../utils/hash_utils.hpp"


/**
 * Callback which checks if a HLookupPair and SPair in an HMap are equal.
 * 
 * @param node1 The HNode contained by the HLookupPair.
 * @param node2 The HNode contained by the SPair.
 * 
 * @return  True if the pairs are equal.
 *          False if not. 
 */
bool is_lookup_pair_equal_to_pair(HNode *node1, HNode *node2) {
    HLookupPair *pair1 = container_of(node1, HLookupPair, node);
    SPair *pair2 = container_of(node2, SPair, map_node);
    if (pair1->len != pair2->len) {
        return false;
    }
    return 0 == memcmp(pair1->name, pair2->name, pair1->len);
}

/**
 * Callback which checks if two SPairs in an HMap are equal.
 * 
 * @param node1 The HNode contained by the first SPair.
 * @param node2 The HNode contained by the second SPair.
 * 
 * @return  True if the pairs are equal.
 *          False if not. 
 */
bool are_pairs_equal(HNode *node1, HNode *node2) {
    SPair *pair1 = container_of(node1, SPair, map_node);
    SPair *pair2 = container_of(node2, SPair, map_node);
    if (pair1->len != pair2->len) {
        return false;
    }
    return 0 == memcmp(pair1->name, pair2->name, pair1->len);
}

/**
 * Callback which compares two SPairs in an AVLTree.
 * 
 * @param node1 The AVLNode contained by the first pair.
 * @param node2 The AVLNode contained by the second pair.
 * 
 * @return  < 0 if first pair < second pair
 *          > 0 if first pair > second pair
 *          0 if the two are equal
 */
int32_t compare_pairs(AVLNode *node1, AVLNode *node2) {
    SPair *pair1 = container_of(node1, SPair, tree_node);
    SPair *pair2 = container_of(node2, SPair, tree_node);
    if (pair1->score != pair2->score) {
        return pair1->score - pair2->score;
    }
    int res = strncmp(pair1->name, pair2->name, std::min(pair1->len, pair2->len));
    return res != 0 ? res : pair1->len - pair2->len;
}

/**
 * Callback which compares an AVLLookupPair and SPair in an AVLTree.
 * 
 * @param node1 The AVLNode contained by the AVLLookupPair.
 * @param node2 The AVLNode contained by the SPair.
 * 
 * @return  < 0 if first pair < second pair
 *          > 0 if first pair > second pair
 *          0 if the two are equal
 */
int32_t compare_lookup_pair_to_pair(AVLNode *node1, AVLNode *node2) {
    AVLLookupPair *pair1 = container_of(node1, AVLLookupPair, node);
    SPair *pair2 = container_of(node2, SPair, tree_node);
    if (pair1->score != pair2->score) {
        return pair1->score - pair2->score;
    }
    int res = strncmp(pair1->name, pair2->name, std::min(pair1->len, pair2->len));
    return res != 0 ? res : pair1->len - pair2->len;
}

/**
 * Cleans up (deallocates) the SPairs in the SortedSet by traversing the AVL tree.
 * 
 * @param root  Pointer to the root node of the SortedSet's AVL tree.
 */
void clean_up_sorted_set(AVLNode *root) {
    if (root == NULL) {
        return;
    }

    clean_up_sorted_set(root->left);
    clean_up_sorted_set(root->right);
    spair_del(container_of(root, SPair, tree_node));
}

SortedSet::~SortedSet() {
    clean_up_sorted_set(tree.root);
}

bool SortedSet::insert(double score, const char *name, uint32_t len) {
    SPair *pair = lookup(name, len);
    if (pair != NULL) {
        update(pair, score);
        return false;
    }
    pair = spair_new(name, len, score);
    map.insert(&pair->map_node);
    tree.insert(&pair->tree_node, compare_pairs);
    return true;
}

SPair *SortedSet::lookup(const char *name, uint32_t len) {
    HLookupPair lookup_pair;
    lookup_pair.node.hval = str_hash(name, len);
    lookup_pair.name = name;
    lookup_pair.len = len;
    HNode *map_node = map.lookup(&lookup_pair.node, is_lookup_pair_equal_to_pair);
    return map_node != NULL ? container_of(map_node, SPair, map_node) : NULL;
}

std::vector<SPair *> SortedSet::find_all_ge(double score, const char *name, uint32_t len, int64_t offset, uint64_t limit) {
    std::vector<SPair *> results;

    SPair *pair = find_first_ge(score, name, len);
    if (pair == NULL) {
        return results;
    }

    pair = find_offset(pair, offset);
    while (pair != NULL && (limit == 0 || results.size() < limit)) {
        results.push_back(pair);
        pair = find_offset(pair, 1);
    }

    return results;
}

bool SortedSet::remove(const char *name, uint32_t len) {
    SPair *pair = lookup(name, len);
    if (pair == NULL) {
        return false;
    }

    map.remove(&pair->map_node, are_pairs_equal);
    tree.root = tree.remove(&pair->tree_node);
    spair_del(pair);

    return true;
}

int64_t SortedSet::rank(const char *name, uint32_t len) {
    SPair *pair = lookup(name, len);
    if (pair == NULL) {
        return -1;
    }
    return tree.rank(&pair->tree_node);
}

uint32_t SortedSet::length() {
    return map.length();
}

void SortedSet::update(SPair *pair, double score) {
    // detach pair from AVLTree
    tree.root = tree.remove(&pair->tree_node);

    // re-insert to fix order
    pair->tree_node = AVLNode(); // reset node data
    pair->score = score;
    tree.insert(&pair->tree_node, compare_pairs);
}

SPair *SortedSet::find_first_ge(double score, const char *name, uint32_t len) {
    AVLLookupPair lookup_pair;
    lookup_pair.score = score;
    lookup_pair.name = name;
    lookup_pair.len = len;
    AVLNode *node = tree.find_first_ge(&lookup_pair.node, compare_lookup_pair_to_pair);
    return node != NULL ? container_of(node, SPair, tree_node) : NULL;
}

SPair *SortedSet::find_offset(SPair *pair, int64_t offset) {
    AVLNode *node = tree.find_offset(&pair->tree_node, offset);
    return node != NULL ? container_of(node, SPair, tree_node) : NULL;
}
