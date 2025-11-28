#include <cstring>
#include <netinet/in.h>
#include <string>

#include "buf_utils.hpp"

void read_uint8(uint8_t *dest, char **src) {
    memcpy(dest, *src, 1);
    *src += 1;
}

void read_uint32(uint32_t *dest, char **src) {
    memcpy(dest, *src, 4);
    *src += 4;
}

void read_int64(int64_t *dest, char **src) {
    memcpy(dest, *src, 8);
    *src += 8;
}

void read_dbl(double *dest, char **src) {
    memcpy(dest, *src, 8);
    *src += 8;
}

void read_str(std::string &dest, uint32_t str_len, char **src) {
    dest.assign(*src, *src + str_len);
    *src += str_len;
}
