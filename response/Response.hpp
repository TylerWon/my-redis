#pragma once

#include <string>
#include <cstdint>
#include <optional>

#include "../buffer/Buffer.hpp"

/* Base response class */
class Response {
    protected:
        static const uint8_t TAG_SIZE = 1;

        /* Identifies the type of response when serialized */
        enum ResponseTag {
            TAG_NIL,
            TAG_ERR,
            TAG_STR,
            TAG_INT,
            TAG_ARR,
            TAG_DBL
        };
    public:
        static const uint8_t HEADER_SIZE = 4;
        static const uint32_t MAX_LEN = 4096;

        enum class MarshalStatus {
            SUCCESS,
            RES_TOO_BIG
        };

        enum class UnmarshalStatus {
            SUCCESS,
            INCOMPLETE_RES,
            RES_TOO_BIG,
            INVALID_RES
        };

        virtual ~Response() = default; // ensures destructor of concrete subclass is called

        /**
         * Marshals the Response into a packet to be sent over the network.
         * 
         * Packet structure:
         * +--------------------+----------------------+
         * | length header (4B) | data (variable size) |
         * +--------------------+----------------------+
         * 
         * @param buf   The Buffer that will store the packet.
         * 
         * @return  SUCCESS on success.
         *          RES_TOO_BIG when the Response exceeds the size limit.
         */
        MarshalStatus marshal(Buffer &buf);

        /**
         * Unmarshals a Response from a response packet in the provided byte buffer.
         * 
         * @param buf   Pointer to a byte buffer that stores the response packet.
         * @param n     Size of the buffer.
         * 
         * @return  (Response, SUCCESS) on success.
         *          (NULL, INCOMPLETE_RES) when the buffer contains an incomplete Response.
         *          (NULL, RES_TOO_BIG) when the Response in the buffer exceeds the size limit.
         *          (NULL, INVALID_RES) when the Response is not one of the Response tags.
         */
        static std::pair<std::optional<Response *>, UnmarshalStatus> unmarshal(char *buf, uint32_t n);

        /**
         * Serializes the Response.
         * 
         * @param buf   The Buffer that will store the serialized Response.
         */
        virtual void serialize(Buffer &buf) = 0;

        /* Returns the length of the Response */
        virtual uint32_t length() = 0;
        
        /* Returns the Response as a string */
        virtual std::string to_string() = 0;
};
