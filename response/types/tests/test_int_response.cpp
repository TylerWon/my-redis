#include <assert.h>
#include <format>

#include "../IntResponse.hpp"
#include "../../../utils/buf_utils.hpp"

void test_serialize() {
    int64_t i = 10;
    IntResponse response(i);
    Buffer buf;

    response.serialize(buf);

    assert(buf.size() == response.length());

    char *p = buf.data();

    uint8_t tag;
    read_uint8(&tag, &p);
    assert(tag == Response::ResponseTag::TAG_INT);

    int64_t res_int;
    read_int64(&res_int, &p);
    assert(res_int == i);

    assert(p - buf.data() == response.length()); // reached end of response
}

void test_deserialize() {
    int64_t i = -100;
    IntResponse response(i);
    Buffer buf;
    response.serialize(buf);

    IntResponse *result = IntResponse::deserialize(buf.data());

    assert(result != NULL);
    assert(result->get_int() == i);
}

void test_to_string() {
    int64_t i = -100;
    IntResponse response(i);
    assert(response.to_string() == std::format("(integer) {}", i));
}

int main() {
    test_serialize();

    test_deserialize();

    test_to_string();

    return 0;
}
