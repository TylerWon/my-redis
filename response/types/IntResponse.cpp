#include <format>

#include "IntResponse.hpp"
#include "../../utils/buf_utils.hpp"

void IntResponse::serialize(Buffer &buf) {
    buf.append_uint8(ResponseTag::TAG_INT);
    buf.append_int64(num);
}

IntResponse* IntResponse::deserialize(char *buf) {
    buf += 1; // skip tag

    int64_t num;
    read_int64(&num, &buf);

    return new IntResponse(num);
}

uint32_t IntResponse::length() {
    return TAG_SIZE + NUM_SIZE;
}

std::string IntResponse::to_string() {
    return std::format("(integer) {}", std::to_string(num));
}

int64_t IntResponse::get_int() {
    return num;
}
