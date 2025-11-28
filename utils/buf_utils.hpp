#include <cstdint>

// Helpers for reading/writing various data types to a buffer for send()/recv()

/**
 * Reads a uint8_t from the src buffer, storing it in dest. Moves the buffer pointer by 1 after.
 * 
 * @param dest  Pointer to a uint8_t where the result will be stored.
 * @param src   Double pointer to a char buffer where the uint8_t will be read from.
 */
void read_uint8(uint8_t *dest, char **src);

/**
 * Reads a uint32_t from the src buffer, storing it in dest. Moves the buffer pointer by 4 after.
 * 
 * @param dest  Pointer to a uint32_t where the result will be stored.
 * @param src   Double pointer to a char buffer where the uint32_t will be read from.
 */
void read_uint32(uint32_t *dest, char **src);

/**
 * Reads a int64_t from the src buffer, storing it in dest. Moves the buffer pointer by 8 after.
 * 
 * @param dest  Pointer a int64_t where the result will be stored.
 * @param src   Double pointer to a char buffer where the int64_t will be read from.
 */
void read_int64(int64_t *dest, char **src);

/**
 * Reads a double from the src buffer, storing it in dest. Moves the buffer pointer by 8 after.
 * 
 * @param dest  Pointer to a double where the result will be stored.
 * @param src   Double pointer to a char buffer where the double will be read from.
 */
void read_dbl(double *dest, char **src);

/**
 * Reads a string of length str_len from the src buffer, storing it in dest. Moves the buffer pointer by str_len after.
 * 
 * @param dest      Reference to a string where the result will be stored.
 * @param str_len   The length of the string to read.
 * @param src       Double pointer to a char buffer where the string will be read from.
 */
void read_str(std::string &dest, uint32_t str_len, char **src);
