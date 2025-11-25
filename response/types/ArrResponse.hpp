#pragma once

#include "../Response.hpp"
#include <vector>

/* An array response */
class ArrResponse : public Response {
    private:
        const uint8_t LEN_SIZE = 4;

        std::vector<Response *> elements;
        uint32_t len;
    public:
        ArrResponse(std::vector<Response *> elements) : elements(elements), len(elements.size()) {};

        ~ArrResponse();

        /**
         * Serialized structure:
         * +----------+-------------+--------------------------+
         * | tag (1B) | length (4B) | elements (variable size) |
         * +----------+-------------+--------------------------+
         * 
         * Note: elements are limited to Response types (i.e. StrResponse, IntResponse, etc.)
         */
        void serialize(Buffer &buf) override;

        /**
         * Deserializes an ArrResponse from the provided byte buffer.
         * 
         * @param buf   Pointer to a byte buffer that stores the ArrResponse.
         * 
         * @return  The Response.
         */
        static ArrResponse* deserialize(const char *buf);
        
        uint32_t length() override;

        std::string to_string() override;
};
