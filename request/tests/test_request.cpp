#include <assert.h>

#include "../Request.hpp"
#include "../../utils/buf_utils.hpp"

void test_marshal_empty_request() {
    Request request({});
    Buffer buf;

    Request::MarshalStatus status = request.marshal(buf);

    assert(status == Request::MarshalStatus::SUCCESS);
    assert(buf.size() == Request::HEADER_SIZE + request.length());

    char *p = buf.data();

    uint32_t req_len;
    read_uint32(&req_len, &p);
    assert(req_len == request.length());

    uint32_t arr_len;
    read_uint32(&arr_len, &p);
    assert(arr_len == 0);
}

void test_marshal_request_too_big() {
    std::vector<std::string> cmd;
    for (uint32_t i = 0; i < Request::MAX_LEN; i++) {
        cmd.push_back(std::to_string(i));
    }
    Request request(cmd);
    Buffer buf;

    Request::MarshalStatus status = request.marshal(buf);

    assert(status == Request::MarshalStatus::REQ_TOO_BIG);
    assert(buf.size() == 0);
}

void test_marshal() {
    std::vector<std::string> cmd = {"set", "name", "tyler"};
    Request request(cmd);
    Buffer buf;

    Request::MarshalStatus status = request.marshal(buf);

    assert(status == Request::MarshalStatus::SUCCESS);
    assert(buf.size() == Request::HEADER_SIZE + request.length());

    char *p = buf.data();

    uint32_t req_len;
    read_uint32(&req_len, &p);
    assert(req_len == request.length());

    uint32_t arr_len;
    read_uint32(&arr_len, &p);
    assert(arr_len == cmd.size());

    for (const std::string &s : cmd) {
        uint32_t str_len;
        read_uint32(&str_len, &p);
        assert(str_len == s.length());

        std::string str;
        read_str(str, str_len, &p);
        assert(str == s);
    }
}

void test_unmarshal_incomplete_request() {
    Request request({"keys"});
    Buffer buf;
    request.marshal(buf);

    auto [req, status] = Request::unmarshal(buf.data(), request.length()-1); // provide buffer length that is less than request length

    assert(status == Request::UnmarshalStatus::INCOMPLETE_REQ);
    assert(req == std::nullopt);
}

void test_unmarshal_request_too_big() {
    Request request({"persist", "school"});
    Buffer buf;
    request.marshal(buf);

    uint32_t *req_len = (uint32_t *) buf.data();
    *req_len = Request::MAX_LEN + 1; // set request length over max size

    auto [req, status] = Request::unmarshal(buf.data(), buf.size());

    assert(status == Request::UnmarshalStatus::REQ_TOO_BIG);
    assert(req == std::nullopt);
}

void test_unmarshal_empty_request() {
    Request request({});
    Buffer buf;
    request.marshal(buf);

    auto [req, status] = Request::unmarshal(buf.data(), buf.size());

    assert(status == Request::UnmarshalStatus::SUCCESS);
    assert(req != std::nullopt);
    
    std::vector<std::string> result = (*req)->get_cmd();
    assert(result.size() == 0);
}

void test_unmarshal() {
    std::vector<std::string> cmd = {"zadd myset 10 tyler"};
    Request request(cmd);
    Buffer buf;
    request.marshal(buf);

    auto [req, status] = Request::unmarshal(buf.data(), buf.size());

    assert(status == Request::UnmarshalStatus::SUCCESS);
    assert(req != std::nullopt);

    std::vector<std::string> result = (*req)->get_cmd();
    assert(result.size() == cmd.size());
    assert(result == cmd);
}

void test_to_string_empty_request() {
    Request request({});
    assert(request.to_string() == "");
}

void test_to_string() {
    Request request({"expire status 100"});
    assert(request.to_string() == "expire status 100");
}

int main() {
    test_marshal_empty_request();
    test_marshal_request_too_big();
    test_marshal();
    
    test_unmarshal_incomplete_request();
    test_unmarshal_request_too_big();
    test_unmarshal_empty_request();
    test_unmarshal();

    test_to_string_empty_request();
    test_to_string();

    return 0;
}
