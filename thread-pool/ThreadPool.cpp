#include <assert.h>

#include "ThreadPool.hpp"
#include "../utils/log.hpp"

ThreadPool::ThreadPool(uint32_t n) {
    pthread_mutex_init(&mu, NULL);
    pthread_cond_init(&not_empty, NULL);
    threads.resize(n);
    for (uint32_t i = 0; i < n; i++) {
        int rv = pthread_create(&threads[i], NULL, &worker, (void *) this);
        assert(rv == 0);
    }
}

ThreadPool::~ThreadPool() {
    pthread_mutex_lock(&mu);
    shutdown = true;
    pthread_cond_broadcast(&not_empty);
    pthread_mutex_unlock(&mu);

    for (pthread_t &t : threads)
        pthread_join(t, NULL);

    pthread_mutex_destroy(&mu);
    pthread_cond_destroy(&not_empty);
}

void ThreadPool::add_task(Task task) {
    pthread_mutex_lock(&mu);
    if (!shutdown) {
        tasks.push_back(task);
        pthread_cond_signal(&not_empty);
    }
    pthread_mutex_unlock(&mu);
}   

void *ThreadPool::worker(void *arg) {
    ThreadPool *tp = (ThreadPool *) arg;
    
    while (true) {
        pthread_mutex_lock(&tp->mu);

        while (tp->tasks.empty() && !tp->shutdown) {
            pthread_cond_wait(&tp->not_empty, &tp->mu); // wait for queue to not be empty
        }

        if (tp->shutdown && tp->tasks.empty()) {
            pthread_mutex_unlock(&tp->mu);
            break;
        }

        Task task = tp->tasks.front();
        tp->tasks.pop_front();
        pthread_mutex_unlock(&tp->mu);

        task.f(task.arg); // perform task (call function)
    }

    return NULL;
}
