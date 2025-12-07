#define TEST_MODE

#include <assert.h>

#include "../CommandExecutor.hpp"
#include "../../response/types/ArrResponse.hpp"
#include "../../response/types/DblResponse.hpp"
#include "../../response/types/ErrResponse.hpp"
#include "../../response/types/IntResponse.hpp"
#include "../../response/types/NilResponse.hpp"
#include "../../response/types/StrResponse.hpp"
#include "../../utils/intrusive_data_structure_utils.hpp"

/**
 * Creates a CommandExecutor.
 */
CommandExecutor *create_executor() {
    HMap *kv_store = new HMap();
    MinHeap *ttl_timers = new MinHeap();
    ThreadPool *thread_pool = new ThreadPool(4);
    return new CommandExecutor(kv_store, ttl_timers, thread_pool);
}

/* Asserts if two Responses are the same. */
void assert_same(std::unique_ptr<Response> &actual, std::unique_ptr<Response> &expected) {
    assert(actual->to_string() == expected->to_string());
}

void test_get_non_existent_key() {
    CommandExecutor *executor = create_executor();

    std::unique_ptr<Response> actual = executor->execute({"get", "name"});
    std::unique_ptr<Response> expected = std::make_unique<NilResponse>();
    assert_same(actual, expected);

    delete executor;
}

void test_get_non_string_entry() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"get", "myset"});
    std::unique_ptr<Response> expected = std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_BAD_TYPE, "value is not a string");
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_get_string_entry() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "name", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"get", "name"});
    std::unique_ptr<Response> expected = std::make_unique<StrResponse>("tyler");
    assert_same(actual, expected);

    executor->execute({"del", "name"});
    delete executor;
}

void test_set_new_key() {
    CommandExecutor *executor = create_executor();

    std::unique_ptr<Response> actual = executor->execute({"set", "name", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<StrResponse>("OK");
    assert_same(actual, expected);

    actual = executor->execute({"get", "name"});
    expected = std::make_unique<StrResponse>("tyler");
    assert_same(actual, expected);

    executor->execute({"del", "name"});
    delete executor;
}

void test_set_existing_entry() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "name", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"set", "name", "won"});
    std::unique_ptr<Response> expected = std::make_unique<StrResponse>("OK");
    assert_same(actual, expected);

    actual = executor->execute({"get", "name"});
    expected = std::make_unique<StrResponse>("won");
    assert_same(actual, expected);

    executor->execute({"del", "name"});
    delete executor;
}

void test_set_existing_entry_with_ttl() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "name", "tyler"});
    executor->execute({"expire", "name", "100"});

    std::unique_ptr<Response> actual = executor->execute({"set", "name", "won"});
    std::unique_ptr<Response> expected = std::make_unique<StrResponse>("OK");
    assert_same(actual, expected);

    actual = executor->execute({"get", "name"});
    expected = std::make_unique<StrResponse>("won");
    assert_same(actual, expected);

    actual = executor->execute({"ttl", "name"});
    expected = std::make_unique<IntResponse>(-1);
    assert_same(actual, expected);

    executor->execute({"del", "name"});
    delete executor;
}

void test_set_existing_non_string_entry() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"set", "myset", "won"});
    std::unique_ptr<Response> expected = std::make_unique<StrResponse>("OK");
    assert_same(actual, expected);

    // check entry is still functional
    actual = executor->execute({"zrem", "myset", "tyler"});
    expected = std::make_unique<IntResponse>(1);
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_del_non_existent_key() {
    CommandExecutor *executor = create_executor();

    std::unique_ptr<Response> actual = executor->execute({"del", "name"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(0);
    assert_same(actual, expected);

    delete executor;
}

void test_del_string_entry() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "name", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"del", "name"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(1);
    assert_same(actual, expected);

    actual = executor->execute({"get", "name"});
    expected = std::make_unique<NilResponse>();
    assert_same(actual, expected);

    delete executor;
}

void test_del_sorted_set_entry() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "10", "tyler"});
    executor->execute({"zadd", "myset", "20", "won"});

    std::unique_ptr<Response> actual = executor->execute({"del", "myset"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(1);
    assert_same(actual, expected);

    actual = executor->execute({"get", "myset"});
    expected = std::make_unique<NilResponse>();
    assert_same(actual, expected);

    delete executor;
}

void test_del_large_sorted_set_entry() {
    CommandExecutor *executor = create_executor();

    for (uint32_t i = 0; i < LARGE_ZSET_SIZE; i++) {
        executor->execute({"zadd", "myset", std::to_string(i), std::to_string(i)});
    }

    std::unique_ptr<Response> actual = executor->execute({"del", "myset"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(1);
    assert_same(actual, expected);

    sleep(1); // sleep briefly to allow async delete to finish

    actual = executor->execute({"get", "myset"});
    expected = std::make_unique<NilResponse>();
    assert_same(actual, expected);

    delete executor;
}

void test_del_entry_with_ttl() {
    // TODO: check ttl_timers is empty?
}

void test_keys_empty_store() {
    CommandExecutor *executor = create_executor();

    std::unique_ptr<Response> actual = executor->execute({"keys"});
    std::unique_ptr<Response> expected = std::make_unique<ArrResponse>(std::vector<Response *>());
    assert_same(actual, expected);

    delete executor;
}

void test_keys_non_empty_store() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "name", "tyler"});
    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"keys"});
    std::vector<Response *> elements = { new StrResponse("myset"), new StrResponse("name") };
    std::unique_ptr<Response> expected = std::make_unique<ArrResponse>(elements);
    assert_same(actual, expected);

    executor->execute({"del", "name"});
    executor->execute({"del", "myset"});
    delete executor;
}

void test_zadd_invalid_score() {
    CommandExecutor *executor = create_executor();

    std::unique_ptr<Response> actual = executor->execute({"zadd", "myset", "ten", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_INVALID_ARG, "invalid score argument");
    assert_same(actual, expected);

    delete executor;
}

void test_zadd_new_key() {
    CommandExecutor *executor = create_executor();

    std::unique_ptr<Response> actual = executor->execute({"zadd", "myset", "10", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(1);
    assert_same(actual, expected);

    actual = executor->execute({"zscore", "myset", "tyler"});
    expected = std::make_unique<StrResponse>(std::to_string(10.0));
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zadd_not_a_sorted_set() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "name", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zadd", "name", "10", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_BAD_TYPE, "value is not a sorted set");
    assert_same(actual, expected);

    executor->execute({"del", "name"});
    delete executor;
}

void test_zadd_new_pair() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zadd", "myset", "20", "won"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(1);
    assert_same(actual, expected);

    actual = executor->execute({"zscore", "myset", "tyler"});
    expected = std::make_unique<StrResponse>(std::to_string(10.0));
    assert_same(actual, expected);

    actual = executor->execute({"zscore", "myset", "won"});
    expected = std::make_unique<StrResponse>(std::to_string(20.0));
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zadd_existing_pair() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zadd", "myset", "20", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(1);
    assert_same(actual, expected);

    actual = executor->execute({"zscore", "myset", "tyler"});
    expected = std::make_unique<StrResponse>(std::to_string(20.0));
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zscore_non_existent_key() {
    CommandExecutor *executor = create_executor();

    std::unique_ptr<Response> actual = executor->execute({"zscore", "myset", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<NilResponse>();
    assert_same(actual, expected);

    delete executor;
}

void test_zscore_not_a_sorted_set() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "myset", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zscore", "myset", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<NilResponse>();
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zscore_non_existent_pair() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zscore", "myset", "won"});
    std::unique_ptr<Response> expected = std::make_unique<NilResponse>();
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zscore_existing_pair() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zscore", "myset", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<StrResponse>(std::to_string(10.0));
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zrem_non_existent_key() {
    CommandExecutor *executor = create_executor();

    std::unique_ptr<Response> actual = executor->execute({"zrem", "myset", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(0);
    assert_same(actual, expected);

    delete executor;
}

void test_zrem_not_a_sorted_set() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "myset", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zrem", "myset", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_BAD_TYPE, "value is not a sorted set");
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zrem_non_existent_pair() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zrem", "myset", "won"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(0);
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zrem_existing_pair() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zrem", "myset", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(1);
    assert_same(actual, expected);

    actual = executor->execute({"zscore", "myset", "tyler"});
    expected = std::make_unique<NilResponse>();
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zquery_invalid_score() {
    CommandExecutor *executor = create_executor();

    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "ten", "tyler", "0", "0"});
    std::unique_ptr<Response> expected = std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_INVALID_ARG, "invalid score argument");
    assert_same(actual, expected);

    delete executor;
}

void test_zquery_invalid_offset() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "10", "tyler", "zero", "0"});
    std::unique_ptr<Response> expected = std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_INVALID_ARG, "invalid offset argument");
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zquery_invalid_limit() {
    CommandExecutor *executor = create_executor();
    
    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "10", "tyler", "0", "zero"});
    std::unique_ptr<Response> expected = std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_INVALID_ARG, "invalid limit argument");
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zquery_non_existent_key() {
    CommandExecutor *executor = create_executor();
    
    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "10", "tyler", "0", "0"});
    std::unique_ptr<Response> expected = std::make_unique<ArrResponse>(std::vector<Response *>());
    assert_same(actual, expected);

    delete executor;
}

void test_zquery_not_a_sorted_set() {
    CommandExecutor *executor = create_executor();
    
    executor->execute({"set", "myset", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "10", "tyler", "0", "0"});
    std::unique_ptr<Response> expected = std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_BAD_TYPE, "value is not a sorted set");
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zquery_no_pairs_with_higher_score() {
    CommandExecutor *executor = create_executor();
    
    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "11", "tyler", "0", "0"});
    std::unique_ptr<Response> expected = std::make_unique<ArrResponse>(std::vector<Response *>());
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zquery_no_pairs_with_higher_name() {
    CommandExecutor *executor = create_executor();
    
    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "10", "won", "0", "0"});
    std::unique_ptr<Response> expected = std::make_unique<ArrResponse>(std::vector<Response *>());
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zquery_pairs_with_higher_score() {
    CommandExecutor *executor = create_executor();
    
    executor->execute({"zadd", "myset", "0", "eve"});
    executor->execute({"zadd", "myset", "10", "tyler"});
    executor->execute({"zadd", "myset", "15", "won"});

    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "5", "adam", "0", "0"});
    std::vector<Response *> elements = { new DblResponse(10.0), new StrResponse("tyler"), new DblResponse(15.0), new StrResponse("won") };
    std::unique_ptr<Response> expected = std::make_unique<ArrResponse>(elements);
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zquery_pairs_with_higher_name() {
    CommandExecutor *executor = create_executor();
    
    executor->execute({"zadd", "myset", "0", "eve"});
    executor->execute({"zadd", "myset", "10", "tyler"});
    executor->execute({"zadd", "myset", "15", "won"});

    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "10", "jeff", "0", "0"});
    std::vector<Response *> elements = { new DblResponse(10.0), new StrResponse("tyler"), new DblResponse(15.0), new StrResponse("won") };
    std::unique_ptr<Response> expected = std::make_unique<ArrResponse>(elements);
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zquery_given_pair_in_sorted_set() {
    CommandExecutor *executor = create_executor();
    
    executor->execute({"zadd", "myset", "0", "eve"});
    executor->execute({"zadd", "myset", "10", "tyler"});
    executor->execute({"zadd", "myset", "15", "won"});

    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "10", "tyler", "0", "0"});
    std::vector<Response *> elements = { new DblResponse(10.0), new StrResponse("tyler"), new DblResponse(15.0), new StrResponse("won") };
    std::unique_ptr<Response> expected = std::make_unique<ArrResponse>(elements);
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zquery_with_offset() {
    CommandExecutor *executor = create_executor();
    
    executor->execute({"zadd", "myset", "0", "eve"});
    executor->execute({"zadd", "myset", "10", "tyler"});
    executor->execute({"zadd", "myset", "15", "won"});

    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "0", "adam", "1", "0"});
    std::vector<Response *> elements = { new DblResponse(10.0), new StrResponse("tyler"), new DblResponse(15.0), new StrResponse("won") };
    std::unique_ptr<Response> expected = std::make_unique<ArrResponse>(elements);
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zquery_offset_skips_all_results() {
    CommandExecutor *executor = create_executor();
    
    executor->execute({"zadd", "myset", "0", "eve"});
    executor->execute({"zadd", "myset", "10", "tyler"});
    executor->execute({"zadd", "myset", "15", "won"});

    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "0", "adam", "3", "0"});
    std::unique_ptr<Response> expected = std::make_unique<ArrResponse>(std::vector<Response *>());
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zquery_under_limit() {
    CommandExecutor *executor = create_executor();
    
    executor->execute({"zadd", "myset", "0", "eve"});
    executor->execute({"zadd", "myset", "10", "tyler"});
    executor->execute({"zadd", "myset", "15", "won"});

    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "10", "tyler", "0", "2"});
    std::vector<Response *> elements = { new DblResponse(10.0), new StrResponse("tyler"), new DblResponse(15.0), new StrResponse("won") };
    std::unique_ptr<Response> expected = std::make_unique<ArrResponse>(elements);
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zquery_hit_limit() {
    CommandExecutor *executor = create_executor();
    
    executor->execute({"zadd", "myset", "0", "eve"});
    executor->execute({"zadd", "myset", "10", "tyler"});
    executor->execute({"zadd", "myset", "15", "won"});

    std::unique_ptr<Response> actual = executor->execute({"zquery", "myset", "10", "tyler", "0", "1"});
    std::vector<Response *> elements = { new DblResponse(10.0), new StrResponse("tyler") };
    std::unique_ptr<Response> expected = std::make_unique<ArrResponse>(elements);
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zrank_non_existent_key() {
    CommandExecutor *executor = create_executor();

    std::unique_ptr<Response> actual = executor->execute({"zrank", "myset", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<NilResponse>();
    assert_same(actual, expected);

    delete executor;
}

void test_zrank_not_a_sorted_set() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "myset", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zrank", "myset", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<NilResponse>();
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zrank_non_existent_pair() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "10", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"zrank", "myset", "won"});
    std::unique_ptr<Response> expected = std::make_unique<NilResponse>();
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zrank_lowest_pair() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "5", "adam"});
    executor->execute({"zadd", "myset", "10", "tyler"});
    executor->execute({"zadd", "myset", "15", "won"});

    std::unique_ptr<Response> actual = executor->execute({"zrank", "myset", "adam"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(0);
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zrank_middle_pair() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "5", "adam"});
    executor->execute({"zadd", "myset", "10", "tyler"});
    executor->execute({"zadd", "myset", "15", "won"});

    std::unique_ptr<Response> actual = executor->execute({"zrank", "myset", "tyler"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(1);
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_zrank_highest_pair() {
    CommandExecutor *executor = create_executor();

    executor->execute({"zadd", "myset", "5", "adam"});
    executor->execute({"zadd", "myset", "10", "tyler"});
    executor->execute({"zadd", "myset", "15", "won"});

    std::unique_ptr<Response> actual = executor->execute({"zrank", "myset", "won"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(2);
    assert_same(actual, expected);

    executor->execute({"del", "myset"});
    delete executor;
}

void test_expire_invalid_duration() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "name", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"expire", "name", "hundred"});
    std::unique_ptr<Response> expected = std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_INVALID_ARG, "invalid seconds argument");
    assert_same(actual, expected);

    executor->execute({"del", "name"});
    delete executor;
}

void test_expire_non_existent_key() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "name", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"expire", "status", "10"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(0);
    assert_same(actual, expected);

    executor->execute({"del", "name"});
    delete executor;
}

void test_expire_existing_key() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "name", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"expire", "name", "10"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(1);
    assert_same(actual, expected);

    actual = executor->execute({"ttl", "name"});
    IntResponse *actual_int = static_cast<IntResponse *>(actual.get());
    assert(actual_int->get_int() > 0);

    executor->execute({"del", "name"});
    delete executor;
}

void test_ttl_non_existent_key() {
    CommandExecutor *executor = create_executor();

    std::unique_ptr<Response> actual = executor->execute({"ttl", "name"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(-2);
    assert_same(actual, expected);

    delete executor;
}

void test_ttl_no_ttl() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "name", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"ttl", "name"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(-1);
    assert_same(actual, expected);

    executor->execute({"del", "name"});
    delete executor;
}

void test_ttl_has_ttl() {
    CommandExecutor *executor = create_executor();

    executor->execute({"set", "name", "tyler"});
    executor->execute({"expire", "name", "10"});

    std::unique_ptr<Response> actual = executor->execute({"ttl", "name"});
    IntResponse *actual_int = static_cast<IntResponse *>(actual.get());
    assert(actual_int->get_int() > 0);

    executor->execute({"del", "name"});
    delete executor;
}

void test_persist_non_existent_key() {
    CommandExecutor *executor = create_executor();

    std::unique_ptr<Response> actual = executor->execute({"persist", "name"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(0);
    assert_same(actual, expected);

    delete executor;
}

void test_persist_no_ttl() {
    CommandExecutor *executor = create_executor();
    
    executor->execute({"set", "name", "tyler"});

    std::unique_ptr<Response> actual = executor->execute({"persist", "name"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(0);
    assert_same(actual, expected);

    executor->execute({"del", "name"});
    delete executor;
}

void test_persist_has_ttl() {
    CommandExecutor *executor = create_executor();
    
    executor->execute({"set", "name", "tyler"});
    executor->execute({"expire", "name", "10"});

    std::unique_ptr<Response> actual = executor->execute({"persist", "name"});
    std::unique_ptr<Response> expected = std::make_unique<IntResponse>(1);
    assert_same(actual, expected);

    actual = executor->execute({"ttl", "name"});
    expected = std::make_unique<IntResponse>(-1);
    assert_same(actual, expected);

    executor->execute({"del", "name"});
    delete executor;
}

void test_invalid_command() {
    CommandExecutor *executor = create_executor();
    
    std::unique_ptr<Response> actual = executor->execute({"not", "a", "command"});
    std::unique_ptr<Response> expected = std::make_unique<ErrResponse>(ErrResponse::ErrorCode::ERR_UNKNOWN, "unknown command");
    assert_same(actual, expected);

    delete executor;
}

int main() {
    test_get_non_existent_key();
    test_get_non_string_entry();
    test_get_string_entry();

    test_set_new_key();
    test_set_existing_entry();
    test_set_existing_entry_with_ttl();
    test_set_existing_non_string_entry();

    test_del_non_existent_key();
    test_del_string_entry();
    test_del_sorted_set_entry();
    test_del_large_sorted_set_entry();
    test_del_entry_with_ttl();

    test_keys_empty_store();
    test_keys_non_empty_store();

    test_zadd_invalid_score();
    test_zadd_new_key();
    test_zadd_not_a_sorted_set();
    test_zadd_new_pair();
    test_zadd_existing_pair();

    test_zscore_non_existent_key();
    test_zscore_not_a_sorted_set();
    test_zscore_non_existent_pair();
    test_zscore_existing_pair();

    test_zrem_non_existent_key();
    test_zrem_not_a_sorted_set();
    test_zrem_non_existent_pair();
    test_zrem_existing_pair();

    test_zquery_invalid_score();
    test_zquery_invalid_offset();
    test_zquery_invalid_limit();
    test_zquery_non_existent_key();
    test_zquery_not_a_sorted_set();
    test_zquery_no_pairs_with_higher_score();
    test_zquery_no_pairs_with_higher_name();
    test_zquery_pairs_with_higher_score();
    test_zquery_pairs_with_higher_name();
    test_zquery_given_pair_in_sorted_set();
    test_zquery_with_offset();
    test_zquery_offset_skips_all_results();
    test_zquery_under_limit();
    test_zquery_hit_limit();

    test_zrank_non_existent_key();
    test_zrank_not_a_sorted_set();
    test_zrank_non_existent_pair();
    test_zrank_lowest_pair();
    test_zrank_middle_pair();
    test_zrank_highest_pair();

    test_expire_invalid_duration();
    test_expire_non_existent_key();
    test_expire_existing_key();

    test_ttl_non_existent_key();
    test_ttl_no_ttl();
    test_ttl_has_ttl();

    test_persist_non_existent_key();
    test_persist_no_ttl();
    test_persist_has_ttl();

    test_invalid_command();

    return 0;
}
