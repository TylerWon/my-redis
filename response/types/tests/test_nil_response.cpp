#include <assert.h>
#include <format>

#include "../NilResponse.hpp"
#include "../../../utils/buf_utils.hpp"

void test_serialize() {
    NilResponse response;
    Buffer buf;

    response.serialize(buf);

    assert(buf.size() == response.length());

    char *p = buf.data();

    uint8_t tag;
    read_uint8(&tag, &p);
    assert(tag == Response::ResponseTag::TAG_NIL);

    assert(p - buf.data() == response.length()); // reached end of response
}

void test_deserialize() {
    NilResponse response;
    Buffer buf;
    response.serialize(buf);

    NilResponse *result = response.deserialize(buf.data());

    assert(result != NULL);
}

void test_to_string() {
    NilResponse response;
    assert(response.to_string() == "(nil)");
}

int main() {
    test_serialize();

    test_deserialize();

    test_to_string();

    return 0;
}
