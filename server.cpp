#include <cstring>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>

#include "command-executor/CommandExecutor.hpp"
#include "conn/Conn.hpp"
#include "constants.hpp"
#include "timers/TimerManager.hpp"
#include "utils/intrusive_data_structure_utils.hpp"
#include "utils/log.hpp"
#include "utils/time_utils.hpp"

HMap kv_store; // key-value store
std::vector<Conn *> fd_to_conn; // map of all client connections, indexed by fd
std::vector<struct pollfd> pollfds; // array of pollfds for poll()
TimerManager timers; // manages idle timers for connections and TTL timers for kv store entries
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
    conn->idle_timer.set_expiry(&timers);

    if (fd_to_conn.size() < (uint32_t) conn->fd) {
        fd_to_conn.resize(conn->fd + 1);
    }

    fd_to_conn[conn->fd] = conn;

    log("new connection %d", client);
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

        if (poll(pollfds.data(), pollfds.size(), timers.get_time_until_expiry()) == -1) {
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
            conn->idle_timer.set_expiry(&timers);

            if (revents & POLLIN) {
                conn->handle_recv(kv_store, timers.ttl_timers, thread_pool);
            }

            if (revents & POLLOUT) {
                conn->handle_send();
            }

            if (revents & POLLERR || conn->want_close) {
                conn->handle_close(fd_to_conn, &timers);
            }
        }

        timers.process_timers(kv_store, fd_to_conn, thread_pool);
    }
}
