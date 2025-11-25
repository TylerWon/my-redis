#include <assert.h>
#include <cstdlib>

#include "HTable.hpp"

HTable::HTable(uint64_t n) {
    assert(n > 0 && ((n - 1) & n) == 0);

    table = (HNode **) calloc(n, sizeof(HNode *)); 
    num_slots = n;
    num_keys = 0;
    mask = n - 1;
}

HTable::~HTable() {
    free(table);
}

void HTable::insert(HNode *node) {
    uint64_t slot = node->hval & mask;
    HNode *head = table[slot];
    node->next = head;
    table[slot] = node;
    num_keys++;
}

HNode **HTable::lookup(HNode *key, bool (*eq)(HNode *, HNode *)) {
    uint64_t slot = key->hval & mask;
    HNode **from = &table[slot];
    for (HNode *curr; (curr = *from) != NULL; from = &curr->next) {
        if (key->hval == curr->hval && eq(key, curr)) {
            return from;
        }
    }
    return NULL;
}

HNode *HTable::detach(HNode **from) {
    HNode *node = *from;
    *from = node->next;
    num_keys--;
    return node;
}

void HTable::for_each(void (*cb)(HNode *, void *), void *cb_arg) {
    for (uint64_t slot = 0; slot < num_slots; slot++) {
        for (HNode *node = table[slot]; node != NULL; node = node->next) {
            cb(node, cb_arg);
        }
    }
}
