#pragma once

#include "../Response.hpp"

/* A Nil response */
class NilResponse : public Response {
    public:
        /**
         * Serialized structure:
         * +----------+
         * | tag (1B) | 
         * +----------+
         */
        void serialize(Buffer &buf) override;

        /**
         * Deserializes a NilResponse from the provided byte buffer.
         * 
         * @param buf   Pointer to a byte buffer that stores the NilResponse.

         * @return  The Response.
         */
        static NilResponse* deserialize(char *buf);
        
        uint32_t length() override;

        std::string to_string() override;
};
