#include "NilResponse.hpp"

void NilResponse::serialize(Buffer &buf) {
    buf.append_uint8(ResponseTag::TAG_NIL);
}

NilResponse* NilResponse::deserialize(char *buf) {
    (void) buf; // suppress unused parameter warning
                // keep unused buf parameter so interface is consistent with other response types
    return new NilResponse();
}

uint32_t NilResponse::length() {
    return TAG_SIZE;
}

std::string NilResponse::to_string() {
    return "(nil)";
}
