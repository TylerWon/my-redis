#include "../conn/Conn.hpp"
#include "../hashmap/HMap.hpp"
#include "../min-heap/MinHeap.hpp"
#include "../queue/Queue.hpp"
#include "../thread-pool/ThreadPool.hpp"

/* Manages idle timers for connections and TTL timers for kv store entries */
class TimerManager {
    private:
        static const uint16_t MAX_TTL_EXPIRATIONS = 1000;
    public:
        Queue idle_timers;
        MinHeap ttl_timers;
    
        /**
         * Gets the time until the next timer expires.
         * 
         * @return  The time until the next timer expires.
         *          0 if the next timer has already expired.
         *          -1 if there are no active timers.
         */
        int32_t get_time_until_expiry();

        /**
         * Checks the idle and TTL timers to see if any have expired.
         * 
         * If a timer has expired, the associated connection or entry is removed.
         * 
         * @param kv_store      Reference to the kv store.
         * @param fd_to_conn    Reference to the map of all connections, indexed by fd.
         * @param thread_pool   Reference to the thread pool used for asynchronous work.
         */
        void process_timers(HMap &kv_store, std::vector<Conn *> &fd_to_conn, ThreadPool &thread_pool);
};
