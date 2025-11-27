#pragma once

#include <cstdint>

/* Dynamic byte buffer with constant time append and remove from front */
class Buffer {
    private:
        char *buffer_start;
        char *buffer_end;
        char *data_start;
        char *data_end;
        
        /* Initializes an n-byte Buffer */
        void init(uint32_t n);
    public:
        /* Initializes a 64 KB Buffer */
        Buffer();

        /* Initializes an n-byte Buffer */
        Buffer(uint32_t n);

        ~Buffer();

        /**
         * Appends n bytes from the provided array to the Buffer.
         * 
         * @param arr   Pointer to a byte array which contains the data to append.
         * @param n     The number of bytes to append to the Buffer from arr.
         */
        void append(const char *arr, uint32_t n);

        /* Appends a uint8_t to the Buffer */
        void append_uint8(uint8_t data);

        /* Appends a uint32_t to the Buffer */
        void append_uint32(uint32_t data);

        /* Appends a int64_t to the Buffer */
        void append_int64(int64_t data);

        /* Appends a double to the Buffer */
        void append_dbl(double data);

        /**
         * Removes n bytes from the front of the Buffer. n can be greater than the number of bytes in the Buffer; this 
         * justs mean the Buffer will be emptied.
         * 
         * @param n The number of bytes to remove from the front of the Buffer.
         */
        void consume(uint32_t n);

        /* Returns a direct pointer to the start of the data in the Buffer */
        char *data();

        /* Returns the number of bytes in the Buffer */
        uint32_t size();
};
