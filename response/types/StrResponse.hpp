#pragma once

#include "../Response.hpp"

/* A string response */
class StrResponse : public Response {
    private:
        static const uint8_t LEN_SIZE = 4;

        std::string msg;
        uint32_t len;
    public:
        StrResponse(std::string msg) : msg(msg), len(msg.length()) {};

        /**
         * Serialized structure:
         * +----------+-------------+------------------------+
         * | tag (1B) | length (4B) | string (variable size) |
         * +----------+-------------+------------------------+
         */
        void serialize(Buffer &buf) override;

        /**
         * Deserializes a StrResponse from the provided byte buffer.
         * 
         * @param buf   Pointer to a byte buffer that stores the StrResponse.
         * 
         * @return  The Response.
         */
        static StrResponse* deserialize(char *buf);
        
        uint32_t length() override;

        /**
         * Format: "(string) <string>"
         * 
         * Example: "(string) this is a sentence"
         */
        std::string to_string() override;

        /* Returns the message */
        std::string get_msg();
};
