#include <format>

#include "ErrResponse.hpp"
#include "../../utils/buf_utils.hpp"

ErrResponse::ErrResponse(ErrorCode code, std::string msg) {
    this->code = code;
    this->str_response = new StrResponse(msg);
}

ErrResponse::~ErrResponse() {
    delete str_response;
}

void ErrResponse::serialize(Buffer &buf) {
    buf.append_uint8(ResponseTag::TAG_ERR);
    buf.append_uint8(code);
    str_response->serialize(buf);
}

ErrResponse* ErrResponse::deserialize(char *buf) {
    buf += 1; // skip tag

    uint8_t code;
    read_uint8(&code, &buf);

    StrResponse *str_response = StrResponse::deserialize(buf);

    return new ErrResponse((ErrorCode) code, str_response);
}

uint32_t ErrResponse::length() {
    return TAG_SIZE + ERR_CODE_SIZE + str_response->length();
}

std::string ErrResponse::to_string() {
    return std::format("(error) {}", str_response->get_msg());
}

ErrResponse::ErrorCode ErrResponse::get_err_code() {
    return code;
}

std::string ErrResponse::get_err_msg() {
    return str_response->get_msg();
}

ErrResponse::ErrResponse(ErrorCode code, StrResponse *str_response) {
    this->code = code;
    this->str_response = str_response;
}
