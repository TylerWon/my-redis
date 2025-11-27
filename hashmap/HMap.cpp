#include <stdlib.h>
#include <assert.h>

#include "HMap.hpp"

HMap::HMap() {
    newer = new HTable(8);
    older = NULL;
    migrate_pos = 0;
}

HMap::~HMap() {
    if (newer != NULL) {
        delete newer;
    }
    if (older != NULL) {
        delete older;
    }
}

void HMap::insert(HNode *node) {
    newer->insert(node);

    if (older == NULL && newer->num_keys >= (newer->num_slots * max_load_factor)) {
        resize();
    }

    rehash_keys();
}

HNode *HMap::lookup(HNode *key, bool (*eq)(HNode *, HNode *)) {
    rehash_keys();

    HNode **from = newer->lookup(key, eq);
    if (from == NULL && older != NULL) {
        from = older->lookup(key, eq);
    }

    return from != NULL ? *from : NULL;
}

HNode *HMap::remove(HNode *key, bool (*eq)(HNode *, HNode *)) {
    rehash_keys();

    HNode **from;
    if ((from = newer->lookup(key, eq)) != NULL) {
        return newer->detach(from);
    }

    if (older != NULL && (from = older->lookup(key, eq)) != NULL) {
        return older->detach(from);
    }

    return NULL;
}

void HMap::for_each(void (*cb)(HNode *, void *), void *cb_arg) {
    newer->for_each(cb, cb_arg);
    if (older != NULL) {
        older->for_each(cb, cb_arg);
    }
}

uint32_t HMap::length() {
    return older != NULL ? newer->num_keys + older->num_keys : newer->num_keys;
}

void HMap::set_max_load_factor(uint32_t max_load_factor) {
    this->max_load_factor = max_load_factor;
}

void HMap::set_num_keys_to_rehash(uint32_t num_keys_to_rehash) {
    this->num_keys_to_rehash = num_keys_to_rehash;
}

void HMap::rehash_keys() {
    if (older == NULL) {
        return;
    }

    uint64_t keys_rehashed = 0;
    while (keys_rehashed < num_keys_to_rehash && older->num_keys > 0) {
        HNode **from = &older->table[migrate_pos];
        if (*from == NULL) {
            migrate_pos++;
            continue;
        }
        newer->insert(older->detach(from));
        keys_rehashed++;
    }

    if (older->num_keys == 0) {
        delete older;
        older = NULL;
    }
}

void HMap::resize() {
    older = newer;
    newer = new HTable(2 * older->num_slots);
    migrate_pos = 0;
}
