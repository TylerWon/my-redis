#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <optional>

#include "../buffer/Buffer.hpp"

/**
 * A request to the Redis server. 
 * 
 * A request is just an array of strings. The strings should form a command when concatenated together.
 */
class Request {
    private:
        static const uint8_t ARR_LEN_SIZE = 4;
        static const uint8_t STR_LEN_SIZE = 4;

        std::vector<std::string> cmd;
        uint32_t len;

        /**
         * Serializes the Request.
         * 
         * Serialized structure:
         * +-------------------+------------------+----------------------+-----+------------------+----------------------+
         * | array length (4B) | str1 length (4B) | str1 (variable size) | ... | strn length (4B) | strn (variable size) |
         * +-------------------+------------------+----------------------+-----+------------------+----------------------+
         * 
         * @param buf   The Buffer that will store the serialized Request.
         */
        void serialize(Buffer &buf);

        /**
         * Deserializes a Request from the provided byte buffer.
         * 
         * @param buf   Pointer to a byte buffer that stores the Request.
         * 
         * @return  The Request.
         */
        static Request* deserialize(char *buf);
    public:
        static const uint32_t MAX_LEN = 4096;
        static const uint8_t HEADER_SIZE = 4;

        enum class MarshalStatus {
            SUCCESS,
            REQ_TOO_BIG
        };

        enum class UnmarshalStatus {
            SUCCESS,
            INCOMPLETE_REQ,
            REQ_TOO_BIG
        };

        /**
         * Initializes the Request with the given Redis command. The command should be provided as an array of strings
         * rather than just a string.
         * 
         * @param cmd   Vector containing the Redis command decomposed into individual strings.
         */
        Request(std::vector<std::string> cmd) : cmd(cmd), len(cmd.size()) {};

        /**
         * Marshals the Request into a packet to be sent over the network. Fails if the request exceeds the size limit.
         * 
         * Packet structure:
         * +--------------------+----------------------+
         * | length header (4B) | data (variable size) |
         * +--------------------+----------------------+
         * 
         * @param buf   The Buffer that will store the packet.
         * 
         * @return  SUCCESS on success.
         *          REQ_TOO_BIG when the Request exceeds the size limit.
         */
        MarshalStatus marshal(Buffer &buf);

        /**
         * Unmarshals a Request from a Request packet in the provided byte buffer. Fails if the request is incomplete
         * (i.e. the request's length header indicates that its longer than the given buffer) or the request exceeds the
         * size limit.
         * 
         * @param buf   Pointer to a byte buffer that stores the Request packet.
         * @param n     Size of the buffer.
         * 
         * @return  (Request, SUCCESS) on success.
         *          (NULL, INCOMPLETE_REQ) when the buffer contains an incomplete Request.
         *          (NULL, REQ_TOO_BIG) when the Request in the buffer exceeds the size limit.
         */
        static std::pair<std::optional<Request *>, UnmarshalStatus> unmarshal(char *buf, uint32_t n);

        /* Returns the length of the Request */
        uint32_t length();
        
        /**
         * Returns the Request as a string.
         * 
         * Example: If command is "set name tyler", returns "set name tyler"
         */
        std::string to_string();

        /* Returns the command */
        std::vector<std::string> get_cmd();
};
