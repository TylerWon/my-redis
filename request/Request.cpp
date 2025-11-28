#include "Request.hpp"
#include "../utils/buf_utils.hpp"

void Request::serialize(Buffer &buf) {
    buf.append_uint32(len);
    for (const std::string &str : cmd) {
        uint32_t str_len = str.length();
        buf.append_uint32(str_len);
        buf.append(str.data(), str_len);
    }
}

Request* Request::deserialize(char *buf) {
    uint32_t len;
    read_uint32(&len, &buf);

    std::vector<std::string> cmd;
    for (uint32_t i = 0; i < len; i++) {
        uint32_t str_len;
        read_uint32(&str_len, &buf);

        std::string str;
        read_str(str, str_len, &buf);

        cmd.push_back(str);
    }

    return new Request(cmd);
}

Request::MarshalStatus Request::marshal(Buffer &buf) {
    if (length() > MAX_LEN) {
        return MarshalStatus::REQ_TOO_BIG;
    }
    buf.append_uint32(length());
    serialize(buf);
    return MarshalStatus::SUCCESS;
}

std::pair<std::optional<Request *>, Request::UnmarshalStatus> Request::unmarshal(char *buf, uint32_t n) {
    if (n < HEADER_SIZE) {
        return std::make_pair(std::nullopt, UnmarshalStatus::INCOMPLETE_REQ);
    }

    uint32_t req_len;
    read_uint32(&req_len, &buf);

    if (req_len > MAX_LEN) {
        return std::make_pair(std::nullopt, UnmarshalStatus::REQ_TOO_BIG);
    } else if (n < HEADER_SIZE + req_len) {
        return std::make_pair(std::nullopt, UnmarshalStatus::INCOMPLETE_REQ);
    }

    return std::make_pair(Request::deserialize(buf), UnmarshalStatus::SUCCESS);
}

uint32_t Request::length() {
    uint32_t str_size = 0;
    for (const std::string &str : cmd) {
        str_size += STR_LEN_SIZE + str.length();
    }
    return LEN_SIZE + str_size;
}

std::string Request::to_string() {
    if (len < 1) {
        return "";
    }

    std::string cmd_str = cmd[0];
    for (uint32_t i = 1; i < len; i++) {
        cmd_str += " ";
        cmd_str += cmd[i];
    }

    return cmd_str;
}

std::vector<std::string> Request::get_cmd() {
    return cmd;
}
