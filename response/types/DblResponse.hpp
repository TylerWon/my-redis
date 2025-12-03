#pragma once

#include "../Response.hpp"

/* A double response */
class DblResponse : public Response {
    private:
        static const uint8_t NUM_SIZE = 8;

        double num;
    public:
        DblResponse(double num) : num(num) {};

        /**
         * Serialized structure:
         * +----------+-------------+
         * | tag (1B) | double (8B) |
         * +----------+-------------+
         */
        void serialize(Buffer &buf) override;

        /**
         * Deserializes a DblResponse from the provided byte buffer.
         * 
         * @param buf   Pointer to a byte buffer that stores the DblResponse.

         * @return  The Response.
         */
        static DblResponse* deserialize(char *buf);
        
        uint32_t length() override;

        /**
         * Format: (double) <double>
         * 
         * Example: "(double) 100.0"
         */
        std::string to_string() override;

        /* Returns the double */
        double get_dbl();
};
