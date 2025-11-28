#include <format>

#include "DblResponse.hpp"
#include "../../utils/buf_utils.hpp"

void DblResponse::serialize(Buffer &buf) {
    buf.append_uint8(ResponseTag::TAG_DBL);
    buf.append_dbl(num);
}

DblResponse* DblResponse::deserialize(char *buf) {
    buf += 1; // skip tag

    double num;
    read_dbl(&num, &buf);

    return new DblResponse(num);
}

uint32_t DblResponse::length() {
    return TAG_SIZE + NUM_SIZE;
}

std::string DblResponse::to_string() {
    return std::format("(double) {}", std::to_string(num));
}

double DblResponse::get_dbl() {
    return num;
}
