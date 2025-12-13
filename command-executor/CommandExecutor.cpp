#include "CommandExecutor.hpp"
#include "../response/types/NilResponse.hpp"
#include "../response/types/StrResponse.hpp"
#include "../response/types/IntResponse.hpp"
#include "../response/types/ErrResponse.hpp"
#include "../response/types/ArrResponse.hpp"
#include "../response/types/DblResponse.hpp"
#include "../utils/hash_utils.hpp"
#include "../utils/intrusive_data_structure_utils.hpp"
#include "../utils/log.hpp"
#include "../utils/time_utils.hpp"

Entry *CommandExecutor::lookup_entry(const std::string &key) {
    LookupEntry lookup_entry;
    lookup_entry.key = key;
    lookup_entry.node.hval = str_hash(key);
    HNode *node = kv_store->lookup(&lookup_entry.node, are_entries_equal);
    return node != NULL ? container_of(node, Entry, node) : NULL;
}

std::unique_ptr<Response> CommandExecutor::do_get(const std::string &key) {
    Entry *entry = lookup_entry(key);

    if (entry == NULL) {
        log("get: key '%s' doesn't exist", key.data());
        return std::make_unique<NilResponse>();
    } else if (entry->type != EntryType::STR) {
        log("get: value of key '%s' isn't a string", key.data());
        return std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_BAD_TYPE, "value is not a string");
    }

    log("get: found key '%s'", key.data());
    return std::make_unique<StrResponse>(entry->str);
}

std::unique_ptr<Response> CommandExecutor::do_set(const std::string &key, const std::string &value) {
    Entry *entry = lookup_entry(key);

    if (entry != NULL) {
        entry->str = value;
        entry->ttl_timer.clear_expiry(timers);
        log("set: updated key '%s'", key.data());
    } else {
        entry = new Entry();
        entry->key = key;
        entry->type = EntryType::STR;
        entry->str = value;
        entry->node.hval = str_hash(key);
        kv_store->insert(&entry->node);
        log("set: created key '%s'", key.data());
    }

    return std::make_unique<StrResponse>("OK");
}

std::unique_ptr<Response> CommandExecutor::do_del(const std::string &key) {
    LookupEntry lookup_entry;
    lookup_entry.key = key;
    lookup_entry.node.hval = str_hash(key);
    HNode *node = kv_store->remove(&lookup_entry.node, are_entries_equal);
    
    if (node != NULL) {
        delete_entry(container_of(node, Entry, node), timers, thread_pool);
        log("del: deleted key '%s'", key.data());
        return std::make_unique<IntResponse>(1);
    }

    log("del: key '%s' doesn't exist", key.data());
    return std::make_unique<IntResponse>(0);
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

std::unique_ptr<Response> CommandExecutor::do_keys() {
    std::vector<std::string> keys;
    kv_store->for_each(get_key, (void *) &keys);

    std::vector<Response *> elements;
    for (const std::string &key : keys) {
        elements.push_back(new StrResponse(key));
    }

    return std::make_unique<ArrResponse>(elements);
}

std::unique_ptr<Response> CommandExecutor::do_zadd(const std::string &key, double score, const std::string &name) {
    Entry *entry = lookup_entry(key);

    if (entry == NULL) {
        entry = new Entry(); // sorted set initialized when Entry created
        entry->key = key;
        entry->type = EntryType::SORTED_SET;
        entry->node.hval = str_hash(key);
        kv_store->insert(&entry->node);
        log("zadd: created sorted set '%s'", key.data());
    } else if (entry != NULL && entry->type != EntryType::SORTED_SET) {
        log("zadd: value of key '%s' isn't a sorted set", key.data());
        return std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_BAD_TYPE, "value is not a sorted set");
    }

    entry->zset.insert(score, name.data(), name.length());
    log("zadd: added pair '(%lf, %s)' to sorted set '%s'", score, name.data(), key.data());

    return std::make_unique<IntResponse>(1);
}

std::unique_ptr<Response> CommandExecutor::do_zscore(const std::string &key, const std::string &name) {
    Entry *entry = lookup_entry(key);

    if (entry == NULL) {
        log("zscore: key '%s' doesn't exist", key.data());
        return std::make_unique<NilResponse>();
    } else if (entry->type != EntryType::SORTED_SET) {
        log("zscore: key '%s' isn't a sorted set", key.data());
        return std::make_unique<NilResponse>();
    } 

    SPair *pair = entry->zset.lookup(name.data(), name.length());
    if (pair == NULL) {
        log("zscore: pair with name '%s' doesn't exist in sorted set '%s'", name.data(), key.data());
        return std::make_unique<NilResponse>();
    }

    log("zscore: found score of name '%s' in sorted set '%s'", name.data(), key.data());
    return std::make_unique<StrResponse>(std::to_string(pair->score));
}

std::unique_ptr<Response> CommandExecutor::do_zrem(const std::string &key, const std::string &name) {
    Entry *entry = lookup_entry(key);

    if (entry == NULL) {
        log("zrem: key '%s' doesn't exist", key.data());
        return std::make_unique<IntResponse>(0);
    } else if (entry != NULL && entry->type != EntryType::SORTED_SET) {
        log("zrem: value of key '%s' isn't a sorted set", key.data());
        return std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_BAD_TYPE, "value is not a sorted set");
    }

    bool success = entry->zset.remove(name.data(), name.length());
    if (success) {
        log("zrem: removed pair with name '%s' from sorted set '%s'", name.data(), key.data());
        return std::make_unique<IntResponse>(1);
    }

    log("zrem: pair with name '%s' doesn't exist in sorted set '%s'", name.data(), key.data());
    return std::make_unique<IntResponse>(0);
}

std::unique_ptr<Response> CommandExecutor::do_zquery(const std::string &key, double score, const std::string &name, uint64_t offset, uint64_t limit) {
    Entry *entry = lookup_entry(key);

    if (entry == NULL) {
        log("zquery: key '%s' doesn't exist", key.data());
        return std::make_unique<ArrResponse>(std::vector<Response *>());
    } else if (entry != NULL && entry->type != EntryType::SORTED_SET) {
        log("zquery: value of key '%s' isn't a sorted set", key.data());
        return std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_BAD_TYPE, "value is not a sorted set");
    }

    std::vector<SPair *> pairs = entry->zset.find_all_ge(score, name.data(), name.length(), offset, limit);
    std::vector<Response *> elements;
    for (const SPair *pair : pairs) {
        Response *score = new DblResponse(pair->score);
        Response *name = new StrResponse(std::string(pair->name, pair->len));
        elements.push_back(score);
        elements.push_back(name);
    }

    log("zquery: got pairs >= '(%lf, %s)' in sorted set '%s'", score, name.data(), key.data());
    return std::make_unique<ArrResponse>(elements);
}

std::unique_ptr<Response> CommandExecutor::do_zrank(const std::string &key, const std::string &name) {
    Entry *entry = lookup_entry(key);
    
    if (entry == NULL) {
        log("zrank: key '%s' doesn't exist", key.data());
        return std::make_unique<NilResponse>();
    } else if (entry->type != EntryType::SORTED_SET) {
        log("zrank: value of key '%s' isn't a sorted set", key.data());
        return std::make_unique<NilResponse>();
    }

    int64_t rank = entry->zset.rank(name.data(), name.length());
    if (rank < 0) {
        log("zrank: pair with name '%s' doesn't exist in sorted set '%s'", name.data(), key.data());
        return std::make_unique<NilResponse>();
    }

    log("zrank: found rank of name '%s' in sorted set '%s'", name.data(), key.data());
    return std::make_unique<IntResponse>(rank);
}

std::unique_ptr<Response> CommandExecutor::do_expire(const std::string &key, time_t seconds) {
    Entry *entry = lookup_entry(key);
    if (entry == NULL) {
        log("expire: key '%s' doesn't exist", key.data());
        return std::make_unique<IntResponse>(0);
    }

    entry->ttl_timer.set_expiry(seconds, timers);
    log("expire: set TTL of key '%s' to %d", key.data(), seconds);
    return std::make_unique<IntResponse>(1);
}

std::unique_ptr<Response> CommandExecutor::do_ttl(const std::string &key) { 
    Entry *entry = lookup_entry(key);
    if (entry == NULL) {
        log("ttl: key '%s' doesn't exist", key.data());
        return std::make_unique<IntResponse>(-2);
    }

    TTLTimer *timer = &entry->ttl_timer;
    if (!timer->is_expiry_set()) {
        log("ttl: key '%s' doesn't have a TTL", key.data());
        return std::make_unique<IntResponse>(-1);
    }

    time_t now_ms = get_time_ms();
    log("ttl: found TTL of key '%s'", key.data());
    return std::make_unique<IntResponse>((timer->expiry_time_ms - now_ms) / 1000);
}

std::unique_ptr<Response> CommandExecutor::do_persist(const std::string &key) { 
    Entry *entry = lookup_entry(key);
    if (entry == NULL) {
        log("persist: key '%s' doesn't exist", key.data());
        return std::make_unique<IntResponse>(0);
    }

    TTLTimer *timer = &entry->ttl_timer;
    if (!timer->is_expiry_set()) {
        log("persist: key '%s' doesn't have a TTL", key.data());
        return std::make_unique<IntResponse>(0);
    }

    timer->clear_expiry(timers);
    log("persist: removed TTL for key '%s'", key.data());
    return std::make_unique<IntResponse>(1);
}

std::unique_ptr<Response> CommandExecutor::execute(const std::vector<std::string> &command) {
    if (command.size() < 1) {
        return std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_UNKNOWN, "unknown command");
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
            uint32_t seconds;
            try {
                seconds = std::stol(command[2]);
            } catch (...) {
                log("expire: invalid seconds argument");
                return std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_INVALID_ARG, "invalid seconds argument");
            }

            return do_expire(command[1], seconds);
        }
    } else if (command.size() == 4) {
        if (name == "zadd") {
            double score;
            try {
                score = std::stod(command[2]);
            } catch (...) {
                log("zadd: invalid score argument");
                return std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_INVALID_ARG, "invalid score argument");
            }

            return do_zadd(command[1], score, command[3]);
        }
    } else if (command.size() == 6) {
        if (name == "zquery") {
            double score;
            try {
                score = std::stod(command[2]);
            } catch (...) {
                log("zquery: invalid score argument");
                return std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_INVALID_ARG, "invalid score argument");
            }

            uint64_t offset;
            try {
                offset = std::stol(command[4]);
            } catch (...) {
                log("zquery: invalid offset argument");
                return std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_INVALID_ARG, "invalid offset argument");
            }

            uint64_t limit;
            try {
                limit = std::stod(command[5]);
            } catch (...) {
                log("zquery: invalid limit argument");
                return std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_INVALID_ARG, "invalid limit argument");
            }
            
            return do_zquery(command[1], score, command[3], offset, limit);
        }
    }
    
    log("request contains unknown command");
    return std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_UNKNOWN, "unknown command");
}
