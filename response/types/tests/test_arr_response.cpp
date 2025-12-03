#include <assert.h>
#include <format>

#include "../ArrResponse.hpp"
#include "../IntResponse.hpp"
#include "../StrResponse.hpp"
#include "../DblResponse.hpp"
#include "../NilResponse.hpp"
#include "../../../utils/buf_utils.hpp"

void test_serialize_empty_array() {
    std::vector<Response *> elements;
    ArrResponse response(elements);
    Buffer buf;
    
    response.serialize(buf);

    assert(buf.size() == response.length());

    char *p = buf.data();

    uint8_t tag;
    read_uint8(&tag, &p);
    assert(tag == Response::ResponseTag::TAG_ARR);

    uint32_t len;
    read_uint32(&len, &p);
    assert(len == elements.size());

    assert(p - buf.data() == response.length()); // reached end of response
}

void test_serialize() {
    std::vector<Response *> elements;
    IntResponse *int_response = new IntResponse(55);
    elements.push_back(int_response);
    StrResponse *str_response = new StrResponse("message");
    elements.push_back(str_response);
    ArrResponse response(elements);
    Buffer buf;
    
    response.serialize(buf);

    assert(buf.size() == response.length());

    char *p = buf.data();

    uint8_t tag;
    read_uint8(&tag, &p);
    assert(tag == Response::ResponseTag::TAG_ARR);

    uint32_t arr_len;
    read_uint32(&arr_len, &p);
    assert(arr_len == elements.size());

    // int
    read_uint8(&tag, &p);
    assert(tag == Response::ResponseTag::TAG_INT);

    int64_t res_int;
    read_int64(&res_int, &p);
    assert(res_int == int_response->get_int());

    // str
    read_uint8(&tag, &p);
    assert(tag == Response::ResponseTag::TAG_STR);

    uint32_t str_len;
    read_uint32(&str_len, &p);
    assert(str_len == str_response->get_msg().length());

    std::string res_str;
    read_str(res_str, str_len, &p);
    assert(res_str == str_response->get_msg());

    assert(p - buf.data() == response.length()); // reached end of response
}

void test_deserialize_empty_array() {
    std::vector<Response *> elements;
    ArrResponse response(elements);
    Buffer buf;
    response.serialize(buf);

    ArrResponse *result = ArrResponse::deserialize(buf.data());

    assert(result != NULL);
    assert(result->get_elements().size() == elements.size());
}

void test_deserialize() {
    std::vector<Response *> elements;
    DblResponse *dbl_response = new DblResponse(0.3);
    elements.push_back(dbl_response);
    NilResponse *nil_response = new NilResponse();
    elements.push_back(nil_response);

    ArrResponse response(elements);
    Buffer buf;
    response.serialize(buf);

    ArrResponse *result = ArrResponse::deserialize(buf.data());

    assert(result != NULL);
    assert(result->get_elements().size() == elements.size());

    Response *element1 = result->get_elements()[0];
    Response *element2 = result->get_elements()[1];
    assert(element1->to_string() == dbl_response->to_string());
    assert(element2->to_string() == nil_response->to_string());
 }

void test_to_string() {
    std::vector<Response *> elements;
    IntResponse *int_response = new IntResponse(55);
    elements.push_back(int_response);
    StrResponse *str_response = new StrResponse("message");
    elements.push_back(str_response);

    ArrResponse response(elements);

    assert(response.to_string() == std::format("(array) len={}\n(integer) {}\n(string) {}\n(array) end", elements.size(), int_response->get_int(), str_response->get_msg()));
}

int main() {
    test_serialize_empty_array();
    test_serialize();

    test_deserialize_empty_array();
    test_deserialize();

    test_to_string();

    return 0;
}
