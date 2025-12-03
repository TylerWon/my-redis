#include <assert.h>
#include <format>

#include "../ErrResponse.hpp"
#include "../../../utils/buf_utils.hpp"
#include <iostream>

void test_serialize() {
    std::string msg = "test";
    ErrResponse::ErrorCode code = ErrResponse::ErrorCode::ERR_BAD_TYPE;
    ErrResponse response(code, msg);
    Buffer buf;
    
    response.serialize(buf);

    assert(buf.size() == response.length());

    char *p = buf.data();

    uint8_t tag;
    read_uint8(&tag, &p);
    assert(tag == Response::ResponseTag::TAG_ERR);

    uint8_t res_code;
    read_uint8(&res_code, &p);
    assert(res_code == code);

    // str
    read_uint8(&tag, &p);
    assert(tag == Response::ResponseTag::TAG_STR);

    uint32_t len;
    read_uint32(&len, &p);
    assert(len == msg.length());

    std::string res_msg;
    read_str(res_msg, len, &p);
    assert(res_msg == msg);

    assert(p - buf.data() == response.length()); // reached end of response
}

void test_deserialize() {
    std::string msg = "this is a message";
    ErrResponse::ErrorCode code = ErrResponse::ErrorCode::ERR_UNKNOWN;
    ErrResponse response(code, msg);
    Buffer buf;
    response.serialize(buf);

    ErrResponse *result = ErrResponse::deserialize(buf.data());

    assert(result != NULL);
    assert(result->get_err_code() == code);
    assert(result->get_err_msg() == msg);
}

void test_to_string() {
    std::string msg = "too big";
    ErrResponse::ErrorCode code = ErrResponse::ErrorCode::ERR_TOO_BIG;
    ErrResponse response(code, msg);
    assert(response.to_string() == std::format("(error) {}", msg));
}

int main() {
    test_serialize();

    test_deserialize();

    test_to_string();

    return 0;
}
