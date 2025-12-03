#include <assert.h>
#include <format>

#include "../StrResponse.hpp"
#include "../../../utils/buf_utils.hpp"

void test_serialize_empty_array() {
    std::string str = "";
    StrResponse response(str);
    Buffer buf;
    
    response.serialize(buf);

    assert(buf.size() == response.length());

    char *p = buf.data();

    uint8_t tag;
    read_uint8(&tag, &p);
    assert(tag == Response::ResponseTag::TAG_STR);

    uint32_t len;
    read_uint32(&len, &p);
    assert(len == str.length());

    assert(p - buf.data() == response.length()); // reached end of response
}

void test_serialize() {
    std::string str = "test";
    StrResponse response(str);
    Buffer buf;
    
    response.serialize(buf);

    assert(buf.size() == response.length());

    char *p = buf.data();

    uint8_t tag;
    read_uint8(&tag, &p);
    assert(tag == Response::ResponseTag::TAG_STR);

    uint32_t len;
    read_uint32(&len, &p);
    assert(len == str.length());

    std::string res_str;
    read_str(res_str, len, &p);
    assert(res_str == str);

    assert(p - buf.data() == response.length()); // reached end of response
}

void test_deserialize() {
    std::string str = "this is a sentence";
    StrResponse response(str);
    Buffer buf;
    response.serialize(buf);

    StrResponse *result = StrResponse::deserialize(buf.data());

    assert(result != NULL);
    assert(result->get_msg() == str);
}

void test_to_string() {
    std::string str = "error: invalid name";
    StrResponse response(str);
    assert(response.to_string() == std::format("(string) {}", str));
}

int main() {
    test_serialize_empty_array();
    test_serialize();

    test_deserialize();

    test_to_string();

    return 0;
}
