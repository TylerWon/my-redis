#include <sys/socket.h>

#include "../command-executor/CommandExecutor.hpp"
#include "Conn.hpp"
#include "../response/types/ErrResponse.hpp"
#include "../utils/log.hpp"

void Conn::handle_send() {
    handle_send_fn(send);
}

void Conn::handle_send_fn(ssize_t (*send)(int fd, const void *buf, size_t n, int flags)) {
    if (!send_data(send)) {
        return;
    }

    if (outgoing.size() == 0) {
        // nothing left to send for connection, change state from write to read 
        want_read = true;
        want_write = false;
    }
}

bool Conn::send_data(ssize_t (*send)(int fd, const void *buf, size_t n, int flags)) {
    int sent = send(fd, outgoing.data(), outgoing.size(), 0);
    if (sent == -1 && errno == EAGAIN) {
        log("connection %d not actually ready to send", fd);
        return false;
    } else if (sent < 0) {
        log("unexpected error when sending on connection %d", fd);
        want_close = true;
        return false;
    } 

    outgoing.consume((uint32_t) sent);

    return true;
}

void Conn::handle_recv(HMap &kv_store, MinHeap &ttl_timers, ThreadPool &thread_pool) {
    handle_recv_fn(kv_store, ttl_timers, thread_pool, recv, send);
}

void Conn::handle_recv_fn(HMap &kv_store, MinHeap &ttl_timers, ThreadPool &thread_pool, ssize_t (*recv)(int fd, void *buf, size_t n, int flags), ssize_t (*send)(int fd, const void *buf, size_t n, int flags)) {
    if (!recv_data(recv)) {
        return;
    }

    CommandExecutor cmd_executor(&kv_store, &ttl_timers, &thread_pool);
    while (Request *request = parse_request()) {
        log("connection %d request: %s", fd, request->to_string().data());

        std::unique_ptr<Response> response = cmd_executor.execute(request->get_cmd());
        if (response->marshal(outgoing) == Response::MarshalStatus::RES_TOO_BIG) {
            log("response to connection %d exceeds the size limit", fd);

            ErrResponse err(ErrResponse::ErrorCode::ERR_TOO_BIG, "response is too big");
            err.marshal(outgoing);
            want_close = true;

            return;
        }

        delete request;
    }

    if (outgoing.size() > 0) {
        // something to send for connection, change state from read to write
        want_read = false;
        want_write = true;
        handle_send_fn(send); // The socket is likely ready to write in a request-response protocol, try to write it 
                              // without waiting for the next iteration
    }
}

bool Conn::recv_data(ssize_t (*recv)(int fd, void *buf, size_t n, int flags)) {
    char buf[64 * 1024]; // 64 KB, large size is to handle pipelined requests
    
    int recvd = recv(fd, buf, sizeof(buf), 0);
    if (recvd == -1 && errno == EAGAIN) {
        log("connection %d not actually ready to receive", fd);
        return false;
    } else if (recvd < 0) {
        log("unexpected error when receiving data for connection %d", fd);
        want_close = true;
        return false;
    } else if (recvd == 0) {
        if (incoming.size() == 0) {
            log("peer terminated connection %d", fd);
        } else {
            log("peer terminated connection %d unexpectedly", fd);
        }
        want_close = true;
        return false;
    }

    incoming.append(buf, (uint32_t) recvd);

    return true;
}

Request *Conn::parse_request() {
    auto [request, status] = Request::unmarshal(incoming.data(), incoming.size());

    if (status == Request::UnmarshalStatus::INCOMPLETE_REQ) {
        return NULL;
    } else if (status == Request::UnmarshalStatus::REQ_TOO_BIG) {
        log("request in connection %d's buffer exceeds the size limit", fd);
        want_close = true;
        return NULL;
    }

    incoming.consume(Request::HEADER_SIZE + (*request)->length());

    return (*request);
}

void Conn::handle_close(std::vector<Conn *> &fd_to_conn, Queue &idle_timers) {
    close(fd);
    idle_timer.clear_expiry(&idle_timers);
    fd_to_conn[fd] = NULL;

    log("closed connection %d", fd);
}
