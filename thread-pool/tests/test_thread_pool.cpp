#include <unistd.h>
#include <assert.h>

#include "../ThreadPool.hpp"

/**
 * Adds one to the given integer.
 * 
 * @param arg   Void pointer to an integer.
 */
void add_one(void *arg) {
    uint32_t *x = (uint32_t *) arg;
    __sync_fetch_and_add(x, 1); // atomic increment for safety
}

/**
 * Sleeps for 1 second.
 * 
 * @param arg   Unused.
 */
void sleep_1(void *arg) {
    (void) arg; // suppress compiler warning
    sleep(1);
}

/* Arguments for producer thread */
struct ProducerArgs {
    uint32_t *counter;
    ThreadPool *tp;
};

/**
 * Producer thread start-up function. Adds 1000 tasks to the thread pool.
 * 
 * @param arg   Void pointer to a ProducerArgs struct.
 */
void *producer(void *arg) {
    ProducerArgs *args = (ProducerArgs *) arg;
    for (uint32_t i = 0; i < 1000; i++) {
        args->tp->add_task({ &add_one, args->counter });
    }
    return NULL;
}

void test_one_producer_one_worker() {
    ThreadPool tp(1);

    uint32_t counter = 0;
    for (uint32_t i = 0; i < 1000; i++) {
        tp.add_task({ &add_one, &counter });
    }

    sleep(1); // small delay to allow execution
    assert(counter == 1000);
}

void test_one_producer_many_workers() {
    ThreadPool tp(4);

    uint32_t counter = 0;
    for (uint32_t i = 0; i < 1000; i++) {
        tp.add_task({ &add_one, &counter });
    }

    sleep(1);
    assert(counter == 1000);
}

void test_many_producers_one_worker() {
    ThreadPool tp(1);
    uint32_t counter = 0;
    
    ProducerArgs args = { &counter, &tp };
    pthread_t t1;
    pthread_create(&t1, NULL, &producer, (void *) &args);
    pthread_t t2;
    pthread_create(&t2, NULL, &producer, (void *) &args);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    sleep(1);
    assert(counter == 2000);
}

void test_many_producers_many_workers() {
    ThreadPool tp(4);
    uint32_t counter = 0;
    
    ProducerArgs args = { &counter, &tp };
    pthread_t t1;
    pthread_create(&t1, NULL, &producer, (void *) &args);
    pthread_t t2;
    pthread_create(&t2, NULL, &producer, (void *) &args);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    sleep(1);
    assert(counter == 2000);
}

void test_task_doesnt_block_other_workers() {
    ThreadPool tp(4);

    // add a slow task, subsequent tasks should still be processed while this task is running
    tp.add_task({ &sleep_1, NULL });

    uint32_t counter = 0;
    for (uint32_t i = 0; i < 1000; i++) {
        tp.add_task({ &add_one, &counter });
    }

    sleep(1);
    assert(counter == 1000);
}

void test_stress_test() {
    ThreadPool tp(4);

    uint32_t counter = 0;
    for (uint32_t i = 0; i < 1000; i++) {
        tp.add_task({ &add_one, &counter });
        if (i % 250 == 0) {
            tp.add_task({ &sleep_1, NULL });
        }
    }

    sleep(2);
    assert(counter == 1000);
}

int main() {
    test_one_producer_one_worker();
    test_one_producer_many_workers();
    test_many_producers_one_worker();
    test_many_producers_many_workers();
    test_task_doesnt_block_other_workers();
    test_stress_test();
    
    return 0;
}