#include <vector>
#include <string>
#include <map>
#include <stdexcept>
#include <format>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <deque>
#include <poll.h>
#include <fcntl.h>

#include "command-executor/CommandExecutor.hpp"
#include "conn/Conn.hpp"
#include "constants.hpp"
#include "entry/Entry.hpp"
#include "hashmap/HMap.hpp"
#include "min-heap/MinHeap.hpp"
#include "queue/Queue.hpp"
#include "request/Request.hpp"
#include "response/Response.hpp"
#include "response/types/NilResponse.hpp"
#include "response/types/StrResponse.hpp"
#include "response/types/IntResponse.hpp"
#include "response/types/ErrResponse.hpp"
#include "response/types/ArrResponse.hpp"
#include "response/types/DblResponse.hpp"
#include "thread-pool/ThreadPool.hpp"
#include "utils/buf_utils.hpp"
#include "utils/intrusive_data_structure_utils.hpp"
#include "utils/hash_utils.hpp"
#include "utils/net_utils.hpp"
#include "utils/log.hpp"
#include "utils/time_utils.hpp"


HMap kv_store; // key-value store
std::vector<Conn *> fd_to_conn; // map of all client connections, indexed by fd
std::vector<struct pollfd> pollfds; // array of pollfds for poll()
Queue idle_timers; // idle timers for active connections, timer closest to expiring is at the front of the queue
MinHeap ttl_timers; // ttl timers for entries in the kv store, timer closest to expiring is at the root of the heap
ThreadPool thread_pool(4); // pool of worker threads for executing asynchronous tasks

/**
 * Gets the address info for the machine running this program which can be used in bind().
 * 
 * @return  Pointer to a struct addrinfo on success. Should be freed when no longer in use.
 *          NULL on error.
 */
struct addrinfo *get_my_addr_info() {
    struct addrinfo *res;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // Stream socket
    hints.ai_flags = AI_PASSIVE;        // Returns wildcard address

    if (getaddrinfo(NULL, PORT, &hints, &res) != 0) {
        return NULL;
    }

    return res;
}

/**
 * Starts the server by creating a listener socket bound to a pre-defined port. 
 * 
 * @param res   Pointer to a struct addrinfo containing the addrinfo for the server.
 * 
 * @return  The listener socket on success.
 *          -1 on error.
 */
int start_server(struct addrinfo *res) {
    struct addrinfo *p;
    int listener;
    for (p = res; p != NULL; p = p->ai_next) {
        if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            log("%s", strerror(errno));
            continue;
        }

        int yes = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));  // Allows port to be re-used

        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
            log("%s", strerror(errno));
            close(listener);
            continue;
        }

        if (listen(listener, SOMAXCONN) == -1) {
            log("%s", strerror(errno));
            close(listener);
            continue;
        }

        return listener;
    }

    return -1;
}

/**
 * Initializes the pollfds array from the map of open connections (fd_to_conn).
 */
void init_pollfds(int listener) {
    // reset from last event loop
    pollfds.clear();
    
    struct pollfd pfd = {listener, POLLIN, 0};
    pollfds.push_back(pfd);

    for (Conn *conn : fd_to_conn) {
        // conn set to NULL when connection is terminated
        if (conn == NULL) {
            continue;
        }

        pfd = {conn->fd, 0, 0};
        if (conn->want_read) {
            pfd.events |= POLLIN;
        }

        if (conn->want_write) {
            pfd.events |= POLLOUT;
        }

        pollfds.push_back(pfd);
    }
}

/**
 * Gets the time until the next timer expires.
 * 
 * @return  The time until the next timer expires.
 *          0 if the next timer has already expired.
 *          -1 if there are no active timers.
 */
int32_t get_time_until_timeout() {
    time_t next_timeout_ms = -1;
    time_t now_ms = get_time_ms();

    if (!idle_timers.is_empty()) {
        QNode *node = idle_timers.front();
        IdleTimer *timer = container_of(node, IdleTimer, node);
        next_timeout_ms = timer->expiry_time_ms;
    }

    if (!ttl_timers.is_empty()) {
        MHNode *node = ttl_timers.min();
        TTLTimer *timer = container_of(node, TTLTimer, node);
        if (next_timeout_ms == -1 || timer->expiry_time_ms < next_timeout_ms) {
            next_timeout_ms = timer->expiry_time_ms;
        }
    }

    if (next_timeout_ms == -1) {
        return -1;
    } else if (now_ms >= next_timeout_ms) {
        return 0;
    }

    return next_timeout_ms - now_ms;
}

/** 
 * Sets a socket so that it is non-blocking.
 * 
 * @param fd    The socket to update.
 * 
 * @return  True on success.
 *          False on error.
 */
bool set_non_blocking(int fd) {
    // get current socket flags
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }

    // Add the O_NONBLOCK flag
    flags |= O_NONBLOCK;

    // update socket flags
    int result = fcntl(fd, F_SETFL, flags);
    if (result == -1) {
        return false;
    }

    return true;
}

/**
 * Handles a new connection on the listener socket.
 * 
 * @param listener  The listener socket.
 */
void handle_new_connection(int listener) {
    int client = accept(listener, NULL, NULL);
    if (client == -1) {
        log("failed to accept new connection");
        return;
    }

    if (!set_non_blocking(client)) {
        log("failed to set socket to non-blocking");
        close(client);
        return;
    }

    Conn *conn = new Conn(client, true, false, false);

    if (fd_to_conn.size() < (uint32_t) conn->fd) {
        fd_to_conn.resize(conn->fd + 1);
    }

    fd_to_conn[conn->fd] = conn;
    idle_timers.push(&conn->idle_timer.node);

    log("new connection %d", client);
}

/**
 * Resets the expiry time of an idle timer.
 * 
 * Since its expiry time is reset, the timer is also moved to the end of the idle timer queue as it will now expire the 
 * latest.
 * 
 * @param timer The timer to reset.
 */
void reset_idle_timer(IdleTimer *timer) {
    timer->reset();
    idle_timers.remove(&timer->node);
    idle_timers.push(&timer->node);
}

/**
 * Checks the idle and TTL timers to see if any have expired.
 * 
 * If a timer has expired, the associated connection/entry is removed.
 */
void process_timers() {
    time_t now_ms = get_time_ms();
    while (!idle_timers.is_empty()) {
        QNode *node = idle_timers.front();
        IdleTimer *timer = container_of(node, IdleTimer, node);
        if (timer->expiry_time_ms > now_ms) {
            break;
        }
        Conn *conn = container_of(timer, Conn, idle_timer);
        log("connection %d exceeded idle timeout", conn->fd);
        conn->handle_close(fd_to_conn, idle_timers);
    }

    const uint32_t MAX_EXPIRATIONS = 1000;
    uint32_t count = 0;
    while (!ttl_timers.is_empty() && count < MAX_EXPIRATIONS) {
        MHNode *node = ttl_timers.min();
        TTLTimer *timer = container_of(node, TTLTimer, node);
        if (timer->expiry_time_ms > now_ms) {
            break;
        }
        Entry *entry = container_of(timer, Entry, ttl_timer);
        log("key \'%s\' expired", entry->key.data());
        kv_store.remove(&entry->node, are_entries_equal);
        delete_entry(entry, &ttl_timers, &thread_pool);
        count++;
    }
}

int main() {
    struct addrinfo *res = get_my_addr_info();
    if (res == NULL) {
        fatal("failed to get server's addrinfo");
    }

    int listener;
    if ((listener = start_server(res)) == -1) {
        fatal("failed to start server");
    }
    freeaddrinfo(res);

    log("started server");

    while (true) {
        init_pollfds(listener);

        if (poll(pollfds.data(), pollfds.size(), get_time_until_timeout()) == -1) {
            fatal("failed to poll");
        }

        // listener socket always at index 0 of pollfds
        if (pollfds[0].revents & POLLIN) {
            handle_new_connection(listener);
        }

        for (uint32_t i = 1; i < pollfds.size(); i++) {
            uint16_t revents = pollfds[i].revents;
            if (revents == 0) {
                continue;
            }
    
            Conn *conn = fd_to_conn[pollfds[i].fd];
            reset_idle_timer(&conn->idle_timer);

            if (revents & POLLIN) {
                conn->handle_recv(kv_store, ttl_timers, thread_pool);
            }

            if (revents & POLLOUT) {
                conn->handle_send();
            }

            if (revents & POLLERR || conn->want_close) {
                conn->handle_close(fd_to_conn, idle_timers);
            }
        }

        process_timers();
    }
}
