#include "CommandExecutor.hpp"
#include "../response/types/NilResponse.hpp"
#include "../response/types/StrResponse.hpp"
#include "../response/types/IntResponse.hpp"
#include "../response/types/ErrResponse.hpp"
#include "../response/types/ArrResponse.hpp"
#include "../response/types/DblResponse.hpp"
#include "../utils/hash_utils.hpp"
#include "../utils/intrusive_data_structure_utils.hpp"
#include "../utils/time_utils.hpp"

Entry *CommandExecutor::lookup_entry(const std::string &key) {
    LookupEntry lookup_entry;
    lookup_entry.key = key;
    lookup_entry.node.hval = str_hash(key);
    HNode *node = kv_store->lookup(&lookup_entry.node, are_entries_equal);
    return node != NULL ? container_of(node, Entry, node) : NULL;
}

Response *CommandExecutor::do_get(const std::string &key) {
    Entry *entry = lookup_entry(key);

    if (entry == NULL) {
        return new NilResponse();
    } else if (entry->type != EntryType::STR) {
        return new ErrResponse(ErrResponse::ErrorCode::ERR_BAD_TYPE, "value is not a string");
    }

    return new StrResponse(entry->str);
}

Response *CommandExecutor::do_set(const std::string &key, const std::string &value) {
    Entry *entry = lookup_entry(key);

    if (entry != NULL) {
        entry->str = value;
    } else {
        entry = new Entry();
        entry->key = key;
        entry->type = EntryType::STR;
        entry->str = value;
        entry->node.hval = str_hash(key);
        if (entry->ttl_timer.expiry_time_ms != 0) {
            entry->ttl_timer.expiry_time_ms = 0;
            ttl_timers->remove(&entry->ttl_timer.node, is_ttl_timer_less);
        }
        kv_store->insert(&entry->node);
    }

    return new StrResponse("OK");
}

Response *CommandExecutor::do_del(const std::string &key) {
    LookupEntry lookup_entry;
    lookup_entry.key = key;
    lookup_entry.node.hval = str_hash(key);
    HNode *node = kv_store->remove(&lookup_entry.node, are_entries_equal);
    
    if (node != NULL) {
        delete_entry(container_of(node, Entry, node), ttl_timers, thread_pool);
        return new IntResponse(1);
    }

    return new IntResponse(0);
}

/**
 * Callback which gets the key for an Entry in the hash map and stores it in the provided vector.
 * 
 * @param node  The HNode contained by the Entry which we want to get the key for.
 * @param arg   Void pointer to a vector to store the key in.
 */
void get_key(HNode *node, void *arg) {
    std::vector<std::string> &keys = *(std::vector<std::string> *) arg;
    Entry *entry = container_of(node, Entry, node);
    keys.push_back(entry->key);
}

Response *CommandExecutor::do_keys() {
    std::vector<std::string> keys;
    kv_store->for_each(get_key, (void *) &keys);

    std::vector<Response *> elements;
    for (const std::string &key : keys) {
        elements.push_back(new StrResponse(key));
    }

    return new ArrResponse(elements);
}

Response *CommandExecutor::do_zadd(const std::string &key, double score, const std::string &name) {
    Entry *entry = lookup_entry(key);

    if (entry == NULL) {
        entry = new Entry(); // sorted set initialized when Entry created
        entry->key = key;
        entry->type = EntryType::SORTED_SET;
        entry->node.hval = str_hash(key);
        kv_store->insert(&entry->node);
    } else if (entry != NULL && entry->type != EntryType::SORTED_SET) {
        return new ErrResponse(ErrResponse::ErrorCode::ERR_BAD_TYPE, "value is not a sorted set");
    }

    entry->zset.insert(score, name.data(), name.length());

    return new IntResponse(1);
}

Response *CommandExecutor::do_zscore(const std::string &key, const std::string &name) {
    Entry *entry = lookup_entry(key);

    if (entry != NULL && entry->type == EntryType::SORTED_SET) {
        SPair *pair = entry->zset.lookup(name.data(), name.length());
        if (pair != NULL) {
            return new StrResponse(std::to_string(pair->score));
        }
    }

    return new NilResponse();
}

Response *CommandExecutor::do_zrem(const std::string &key, const std::string &name) {
    Entry *entry = lookup_entry(key);

    if (entry == NULL) {
        return new IntResponse(0);
    } else if (entry != NULL && entry->type != EntryType::SORTED_SET) {
        return new ErrResponse(ErrResponse::ErrorCode::ERR_BAD_TYPE, "value is not a sorted set");
    }

    bool success = entry->zset.remove(name.data(), name.length());

    return success ? new IntResponse(1) : new IntResponse(0);
}

Response *CommandExecutor::do_zquery(const std::string &key, double score, const std::string &name, uint64_t offset, uint64_t limit) {
    Entry *entry = lookup_entry(key);

    if (entry == NULL) {
        return new ArrResponse(std::vector<Response *>());
    } else if (entry != NULL && entry->type != EntryType::SORTED_SET) {
        return new ErrResponse(ErrResponse::ErrorCode::ERR_BAD_TYPE, "value is not a sorted set");
    }

    std::vector<SPair *> pairs = entry->zset.find_all_ge(score, name.data(), name.length(), offset, limit);
    std::vector<Response *> elements;
    for (const SPair *pair : pairs) {
        Response *score = new DblResponse(pair->score);
        Response *name = new StrResponse(std::string(pair->name, pair->len));
        elements.push_back(score);
        elements.push_back(name);
    }

    return new ArrResponse(elements);
}

Response *CommandExecutor::do_zrank(const std::string &key, const std::string &name) {
    Entry *entry = lookup_entry(key);
    
    if (entry == NULL || entry->type != EntryType::SORTED_SET) {
        return new NilResponse();
    }

    int64_t rank = entry->zset.rank(name.data(), name.length());
    if (rank < 0) {
        return new NilResponse();
    }

    return new IntResponse(rank);
}

Response *CommandExecutor::do_expire(const std::string &key, time_t seconds) {
    Entry *entry = lookup_entry(key);
    if (entry == NULL) {
        return new IntResponse(0);
    }

    TTLTimer *timer = &entry->ttl_timer;
    time_t old_expiry_time = timer->expiry_time_ms;
    timer->expiry_time_ms = get_time_ms() + seconds * 1000;
    if (old_expiry_time == 0) {
        ttl_timers->insert(&timer->node, is_ttl_timer_less);
    } else {
        ttl_timers->update(&timer->node, is_ttl_timer_less);
    }

    return new IntResponse(1);
}

Response *CommandExecutor::do_ttl(const std::string &key) { 
    Entry *entry = lookup_entry(key);
    if (entry == NULL) {
        return new IntResponse(-2);
    }

    TTLTimer *timer = &entry->ttl_timer;
    if (timer->expiry_time_ms == 0) {
        return new IntResponse(-1);
    }

    time_t now_ms = get_time_ms();
    return new IntResponse((timer->expiry_time_ms - now_ms) / 1000);
}

Response *CommandExecutor::do_persist(const std::string &key) { 
    Entry *entry = lookup_entry(key);
    if (entry == NULL) {
        return new IntResponse(0);
    }

    TTLTimer *timer = &entry->ttl_timer;
    if (timer->expiry_time_ms == 0) {
        return new IntResponse(0);
    }

    ttl_timers->remove(&timer->node, is_ttl_timer_less);
    timer->expiry_time_ms = 0;

    return new IntResponse(1);
}

Response *CommandExecutor::execute(const std::vector<std::string> &command) {
    if (command.size() < 1) {
        return new ErrResponse(ErrResponse::ErrorCode::ERR_UNKNOWN, "unknown command");
    }

    std::string name = command[0];
    if (command.size() == 1) {
        if (name == "keys") {
            return do_keys();
        }
    } else if (command.size() == 2) {
        if (name == "get") {
            return do_get(command[1]);
        } else if (name == "del") {
            return do_del(command[1]);
        } else if (name == "ttl") {
            return do_ttl(command[1]);
        } else if (name == "persist") {
            return do_persist(command[1]);
        }
    } else if (command.size() == 3) {
        if (name == "set") {
            return do_set(command[1], command[2]);
        } else if (name == "zscore") {
            return do_zscore(command[1], command[2]);
        } else if (name == "zrem") {
            return do_zrem(command[1], command[2]);
        } else if (name == "zrank") {
            return do_zrank(command[1], command[2]); 
        } else if (name == "expire") {
            return do_expire(command[1], std::stol(command[2]));
        }
    } else if (command.size() == 4) {
        if (name == "zadd") {
            return do_zadd(command[1], std::stod(command[2]), command[3]);
        }
    } else if (command.size() == 6) {
        if (name == "zquery") {
            return do_zquery(command[1], std::stod(command[2]), command[3], std::stol(command[4]), std::stoul(command[5]));
        }
    }
    
    return new ErrResponse(ErrResponse::ErrorCode::ERR_UNKNOWN, "unknown command");
}
