#include <cstddef>
#include <vector>
#include <deque>
#include <pthread.h>
#include <cstdint>

/* A task to be executed by a worker thread */
struct Task {
    void (*f)(void *) = NULL;
    void *arg = NULL;
};

/**
 * A collection of worker threads that execute tasks from a queue. 
 * 
 * An unspecified number of producers can add tasks to the queue.
 */
class ThreadPool {
    private:
        std::vector<pthread_t> threads;
        std::deque<Task> tasks;
        pthread_mutex_t mu;
        pthread_cond_t not_empty;
    public:
        /* Initializes the ThreadPool with n workers */
        ThreadPool(uint32_t n);

        /**
         * Adds a Task to the queue.
         * 
         * @param Task  The Task to add to the queue.
         */
        void add_task(Task task);

        /**
         * Start-up function for a worker thread. 
         * 
         * The worker retrieves and processes tasks from the queue while it is not empty. Otherwise, it waits (sleeps)
         * until there are tasks available.
         * 
         * @param arg    Void pointer to the ThreadPool.
         */
        static void *worker(void *arg);
};
