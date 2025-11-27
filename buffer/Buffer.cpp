#include <cstdlib>
#include <cstring>
#include <algorithm>

#include "../utils/log.hpp"
#include "Buffer.hpp"

Buffer::Buffer() {
    init(64 * 1024);
}

Buffer::Buffer(uint32_t n) {
    init(n);
}

Buffer::~Buffer() {
    free(buffer_start);
}

void Buffer::append(const char *arr, uint32_t n) {
    uint32_t space_at_start = data_start - buffer_start;
    uint32_t space_at_end = buffer_end - data_end;
    uint32_t data_size = data_end - data_start;
    uint32_t buffer_size = buffer_end - buffer_start;

    if (n <= space_at_end) {
        // enough space at end
        memcpy(data_end, arr, n);
        data_end += n;
    } else if (n <= space_at_start + space_at_end) {
        // enough space, but need to move current data to the front to make room
        memcpy(buffer_start, data_start, data_size);
        data_start = buffer_start;
        data_end = data_start + data_size;
        memcpy(data_end, arr, n);
        data_end += n;
    } else {
        // not enough space, need to resize
        buffer_start = (char *) realloc(buffer_start, 2 * buffer_size);
        buffer_end = buffer_start + 2 * buffer_size;
        data_start = buffer_start + space_at_start;
        data_end = data_start + data_size;
        append(arr, n);
    }
}

void Buffer::append_uint8(uint8_t data) {
    append((char *) &data, 1);
}

void Buffer::append_uint32(uint32_t data) {
    append((char *) &data, 4);
}

void Buffer::append_int64(int64_t data) {
    append((char *) &data, 8);
}

void Buffer::append_dbl(double data) {
    append((char *) &data, 8);
}

void Buffer::consume(uint32_t n) {
    if (size() == 0) {
        log("nothing to remove from Buffer");
        return;
    }
    data_start += std::min(n, size());
}

char *Buffer::data() {
    return data_start;
}

uint32_t Buffer::size() {
    return data_end - data_start;
}

void Buffer::init(uint32_t n) {
    buffer_start = (char *) malloc(n);
    buffer_end = buffer_start + n;
    data_start = buffer_start;
    data_end = data_start;
}
