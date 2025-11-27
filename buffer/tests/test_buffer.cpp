#include <cstring>
#include <assert.h>

#include "../Buffer.hpp"

void test_append() {
    Buffer buf(4);

    const char *word = "test";
    uint32_t n = strlen(word);
    buf.append(word, n);

    assert(buf.size() == n);
    assert(strncmp(buf.data(), word, n) == 0);
}

void test_append_shift_then_append() {
    Buffer buf(4);

    const char *word = "test";
    buf.append(word, strlen(word));
    buf.consume(2);
    word = "ep";
    buf.append(word, strlen(word));

    assert(buf.size() == 4);
    assert(strncmp(buf.data(), "step", 4) == 0);
}

void test_append_resize_then_append() {
    Buffer buf(4);

    const char *word = "test";
    buf.append(word, strlen(word));
    word = "ing";
    buf.append(word, strlen(word));

    assert(buf.size() == 7);
    assert(strncmp(buf.data(), "testing", 7) == 0);
}

void test_append_uint8() {
    Buffer buf(4);

    uint8_t num = 10;
    buf.append_uint8(num);

    assert(buf.size() == 1);
    assert(*((uint8_t *) buf.data()) == num);
}

void test_append_uint32() {
    Buffer buf(4);

    uint32_t num = 100;
    buf.append_uint32(num);

    assert(buf.size() == 4);
    assert(*((uint32_t *) buf.data()) == num);
}

void test_append_int64() {
    Buffer buf(4);

    int64_t num = -1000;
    buf.append_int64(num);

    assert(buf.size() == 8);
    assert(*((int64_t *) buf.data()) == num);
}

void test_append_dbl() {
    Buffer buf(4);

    double num = 10000.0;
    buf.append_dbl(num);

    assert(buf.size() == 8);
    assert(*((double *) buf.data()) == num);
}

void test_consume() {
    Buffer buf(4);

    const char *word = "test";
    buf.append(word, strlen(word));
    buf.consume(2);

    assert(buf.size() == 2);
    assert(strncmp(buf.data(), "st", 2) == 0);
}

void test_consume_exceeds_buffer_size() {
    Buffer buf(4);

    const char *word = "test";
    buf.append(word, strlen(word));
    buf.consume(5);

    assert(buf.size() == 0);
}


int main() {
    test_append();
    test_append_shift_then_append();
    test_append_resize_then_append();

    test_append_uint8();
    test_append_uint32();
    test_append_int64();
    test_append_dbl();

    test_consume();
    test_consume_exceeds_buffer_size();

    return 0;
}
