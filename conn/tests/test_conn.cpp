#define TEST_MODE

#include <assert.h>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>

#include "../Conn.hpp"
#include "../../entry/Entry.hpp"
#include "../../queue/Queue.hpp"
#include "../../response/Response.hpp"
#include "../../response/types/StrResponse.hpp"
#include "../../utils/intrusive_data_structure_utils.hpp"
#include "../../utils/hash_utils.hpp"

StrResponse test_response("this is a message");

Request test_request1({"set", "name", "tyler"});
StrResponse test_request1_response("OK");

Request test_request2({"get", "name"});
StrResponse test_request2_response("tyler");

// send mocks
ssize_t send_test_handle_send_socket_not_ready(int fd, const void *buf, size_t n, int flags) {
    (void) fd;
    (void) buf;
    (void) n;
    (void) flags;

    errno = EAGAIN;
    return -1;
}

ssize_t send_test_handle_send_unexpected_error(int fd, const void *buf, size_t n, int flags) {
    (void) fd;
    (void) buf;
    (void) n;
    (void) flags;
    
    errno = ENOMEM;
    return -1;
}

ssize_t send_test_handle_send_all_data_sent(int fd, const void *buf, size_t n, int flags) {
    (void) fd;
    (void) buf;
    (void) n;
    (void) flags;
    
    return Response::HEADER_SIZE + test_response.length();
}

ssize_t send_test_handle_send_some_data_sent(int fd, const void *buf, size_t n, int flags) {
    (void) fd;
    (void) buf;
    (void) n;
    (void) flags;
    
    // subtract a few bytes to simulate partial send of response
    return Response::HEADER_SIZE + test_response.length() - 5;
}

ssize_t send_test_handle_recv_one_request(int fd, const void *buf, size_t n, int flags) {
    (void) fd;
    (void) buf;
    (void) n;
    (void) flags;
    
    return Response::HEADER_SIZE + test_request1_response.length();
}

ssize_t send_test_handle_recv_multiple_requests(int fd, const void *buf, size_t n, int flags) {
    (void) fd;
    (void) buf;
    (void) n;
    (void) flags;
    
    return Response::HEADER_SIZE + test_request1_response.length() + Response::HEADER_SIZE + test_request2_response.length();
}

// recv mocks
ssize_t recv_test_handle_recv_socket_not_ready(int fd, void *buf, size_t n, int flags) {
    (void) fd;
    (void) buf;
    (void) n;
    (void) flags;

    errno = EAGAIN;
    return -1;
}

ssize_t recv_test_handle_recv_unexpected_error(int fd, void *buf, size_t n, int flags) {
    (void) fd;
    (void) buf;
    (void) n;
    (void) flags;

    errno = ENOTCONN;
    return -1;
}

ssize_t recv_test_handle_recv_peer_terminated_connection(int fd, void *buf, size_t n, int flags) {
    (void) fd;
    (void) buf;
    (void) n;
    (void) flags;

    return 0;
}

ssize_t recv_test_handle_recv_request_too_big(int fd, void *buf, size_t n, int flags) {
    (void) fd;
    (void) n;
    (void) flags;

    // Request class doesn't allow a request that exceeds the max size to be marshalled so write directly to the buffer
    uint32_t req_len = Request::MAX_LEN + 1;
    mempcpy(buf, &req_len, Request::HEADER_SIZE);
    memset((char *) buf + Request::HEADER_SIZE, 1, req_len);

    return Request::HEADER_SIZE + req_len;
}

ssize_t recv_test_handle_recv_incomplete_request(int fd, void *buf, size_t n, int flags) {
    (void) fd;
    (void) n;
    (void) flags;

    Buffer temp;
    test_request1.marshal(temp);
    memcpy(buf, temp.data(), temp.size());

    // subtract a few bytes to simulate partial recv of request
    return Request::HEADER_SIZE + test_request1.length() - 10;
}

ssize_t recv_test_handle_recv_one_request(int fd, void *buf, size_t n, int flags) {
    (void) fd;
    (void) n;
    (void) flags;

    Buffer temp;
    test_request1.marshal(temp);
    memcpy(buf, temp.data(), temp.size());

    return Request::HEADER_SIZE + test_request1.length();
}

ssize_t recv_test_handle_recv_multiple_requests(int fd, void *buf, size_t n, int flags) {
    (void) fd;
    (void) n;
    (void) flags;

    Buffer temp;
    test_request1.marshal(temp);
    test_request2.marshal(temp);
    memcpy(buf, temp.data(), temp.size());

    return Request::HEADER_SIZE + test_request1.length() + Request::HEADER_SIZE + test_request2.length();
}

/**
 * Asserts that a key is in the kv store.
 * 
 * @param key       The key to look for.
 * @param kv_store  The kv store.
 */
void assert_key_in_store(std::string key, HMap &kv_store) {
    LookupEntry lookup_entry;
    lookup_entry.key = key;
    lookup_entry.node.hval = str_hash(key);
    HNode *node = kv_store.lookup(&lookup_entry.node, are_entries_equal);
    assert(node != NULL);
}

void test_handle_send_socket_not_ready() {
    Conn conn(10, false, true, false);
    test_response.marshal(conn.outgoing);

    conn.handle_send_fn(send_test_handle_send_socket_not_ready);

    assert(conn.outgoing.size() == Response::HEADER_SIZE + test_response.length());
    assert(conn.want_read == false);
    assert(conn.want_write == true);
    assert(conn.want_close == false);
}

void test_handle_send_unexpected_error() {
    Conn conn(10, false, true, false);
    test_response.marshal(conn.outgoing);

    conn.handle_send_fn(send_test_handle_send_unexpected_error);

    assert(conn.outgoing.size() == Response::HEADER_SIZE + test_response.length());
    assert(conn.want_read == false);
    assert(conn.want_write == true);
    assert(conn.want_close == true);
}

void test_handle_send_all_data_sent()  {
    Conn conn(10, false, true, false);
    test_response.marshal(conn.outgoing);

    conn.handle_send_fn(send_test_handle_send_all_data_sent);

    assert(conn.outgoing.size() == 0);
    assert(conn.want_read == true);
    assert(conn.want_write == false);
    assert(conn.want_close == false);
}

void test_handle_send_some_data_sent()  {
    Conn conn(10, false, true, false);
    test_response.marshal(conn.outgoing);

    conn.handle_send_fn(send_test_handle_send_some_data_sent);

    assert(conn.outgoing.size() == 5);
    assert(conn.want_read == false);
    assert(conn.want_write == true);
    assert(conn.want_close == false);
}

void test_handle_recv_socket_not_ready() {
    HMap kv_store;
    MinHeap ttl_timers;
    ThreadPool thread_pool(4);
    Conn conn(10, true, false, false);

    conn.handle_recv_fn(kv_store, ttl_timers, thread_pool, recv_test_handle_recv_socket_not_ready, send);

    assert(conn.incoming.size() == 0);
    assert(conn.want_read == true);
    assert(conn.want_write == false);
    assert(conn.want_close == false);
}

void test_handle_recv_unexpected_error() {
    HMap kv_store;
    MinHeap ttl_timers;
    ThreadPool thread_pool(4);
    Conn conn(10, true, false, false);

    conn.handle_recv_fn(kv_store, ttl_timers, thread_pool, recv_test_handle_recv_unexpected_error, send);

    assert(conn.incoming.size() == 0);
    assert(conn.want_read == true);
    assert(conn.want_write == false);
    assert(conn.want_close == true);
}

void test_handle_recv_peer_terminated_connection() {
    HMap kv_store;
    MinHeap ttl_timers;
    ThreadPool thread_pool(4);
    Conn conn(10, true, false, false);

    conn.handle_recv_fn(kv_store, ttl_timers, thread_pool, recv_test_handle_recv_peer_terminated_connection, send);

    assert(conn.incoming.size() == 0);
    assert(conn.want_read == true);
    assert(conn.want_write == false);
    assert(conn.want_close == true);
}

void test_handle_recv_request_too_big() {
    HMap kv_store;
    MinHeap ttl_timers;
    ThreadPool thread_pool(4);
    Conn conn(10, true, false, false);

    conn.handle_recv_fn(kv_store, ttl_timers, thread_pool, recv_test_handle_recv_request_too_big, send);

    assert(conn.incoming.size() == Request::HEADER_SIZE + Request::MAX_LEN + 1);
    assert(conn.want_read == true);
    assert(conn.want_write == false);
    assert(conn.want_close == true);
}

void test_handle_recv_incomplete_request() {
    HMap kv_store;
    MinHeap ttl_timers;
    ThreadPool thread_pool(4);
    Conn conn(10, true, false, false);

    conn.handle_recv_fn(kv_store, ttl_timers, thread_pool, recv_test_handle_recv_incomplete_request, send);

    assert(conn.incoming.size() == Request::HEADER_SIZE + test_request1.length() - 10);
    assert(conn.want_read == true);
    assert(conn.want_write == false);
    assert(conn.want_close == false);
}

void test_handle_recv_one_request() {
    HMap kv_store;
    MinHeap ttl_timers;
    ThreadPool thread_pool(4);
    Conn conn(10, true, false, false);

    conn.handle_recv_fn(kv_store, ttl_timers, thread_pool, recv_test_handle_recv_one_request, send_test_handle_recv_one_request);

    assert(conn.incoming.size() == 0);
    assert(conn.outgoing.size() == 0); // response sent
    assert(conn.want_read == true);
    assert(conn.want_write == false);
    assert(conn.want_close == false);
    assert_key_in_store("name", kv_store);
}

void test_handle_recv_multiple_requests() {
    HMap kv_store;
    MinHeap ttl_timers;
    ThreadPool thread_pool(4);
    Conn conn(10, true, false, false);

    conn.handle_recv_fn(kv_store, ttl_timers, thread_pool, recv_test_handle_recv_multiple_requests, send_test_handle_recv_multiple_requests);

    assert(conn.incoming.size() == 0);
    assert(conn.outgoing.size() == 0); // responses sent
    assert(conn.want_read == true);
    assert(conn.want_write == false);
    assert(conn.want_close == false);
    assert_key_in_store("name", kv_store);
}

void test_handle_close() {
    Conn conn(10, true, false, false);
    std::vector<Conn *> fd_to_conn(conn.fd + 1);
    Queue idle_timers;
    fd_to_conn[conn.fd] = &conn; // insert Conn at index fd
    idle_timers.push(&conn.idle_timer.node);

    conn.handle_close(fd_to_conn, idle_timers);

    assert(fd_to_conn[conn.fd] == NULL);
    assert(idle_timers.is_empty() == true);
}

int main() {
    test_handle_send_socket_not_ready();
    test_handle_send_unexpected_error();
    test_handle_send_all_data_sent();
    test_handle_send_some_data_sent();

    test_handle_recv_socket_not_ready();
    test_handle_recv_unexpected_error();
    test_handle_recv_peer_terminated_connection();
    test_handle_recv_request_too_big();
    test_handle_recv_incomplete_request();
    test_handle_recv_one_request();
    test_handle_recv_multiple_requests();

    test_handle_close();
    
    return 0;
}
