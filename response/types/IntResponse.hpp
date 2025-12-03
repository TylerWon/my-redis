#pragma once

#include "../Response.hpp"

/* An integer response */
class IntResponse : public Response {
    private:
        static const uint8_t NUM_SIZE = 8;

        int64_t num;
    public:
        IntResponse(int64_t num) : num(num) {};

        /**
         * Serialized structure:
         * +----------+--------------+
         * | tag (1B) | integer (8B) |
         * +----------+--------------+
         */
        void serialize(Buffer &buf) override;

        /**
         * Deserializes an IntResponse from the provided byte buffer.
         * 
         * @param buf   Pointer to a byte buffer that stores the IntResponse.

         * @return  The Response.
         */
        static IntResponse* deserialize(char *buf);
        
        uint32_t length() override;

        /**
         * Format: (integer) <integer>
         * 
         * Example: "(integer) 10"
         */
        std::string to_string() override;

        /* Returns the integer */
        int64_t get_int();
};
