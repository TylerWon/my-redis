#include <assert.h>
#include <format>

#include "../DblResponse.hpp"
#include "../../../utils/buf_utils.hpp"

void test_serialize() {
    double dbl = 10.0;
    DblResponse response(dbl);
    Buffer buf;

    response.serialize(buf);

    assert(buf.size() == response.length());

    char *p = buf.data();

    uint8_t tag;
    read_uint8(&tag, &p);
    assert(tag == Response::ResponseTag::TAG_DBL);

    double res_dbl;
    read_dbl(&res_dbl, &p);
    assert(res_dbl == dbl);

    assert(p - buf.data() == response.length()); // reached end of response
}

void test_deserialize() {
    double dbl = -100.5;
    DblResponse response(dbl);
    Buffer buf;
    response.serialize(buf);

    DblResponse *result = DblResponse::deserialize(buf.data());

    assert(result != NULL);
    assert(result->get_dbl() == dbl);
}

void test_to_string() {
    double dbl = 3.99;
    DblResponse response(dbl);
    assert(response.to_string() == std::format("(double) {}", std::to_string(dbl)));
}

int main() {
    test_serialize();

    test_deserialize();

    test_to_string();

    return 0;
}
