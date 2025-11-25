#include <format>

#include "ArrResponse.hpp"
#include "NilResponse.hpp"
#include "StrResponse.hpp"
#include "ErrResponse.hpp"
#include "IntResponse.hpp"
#include "DblResponse.hpp"
#include "../../utils/buf_utils.hpp"
#include "../../utils/log.hpp"

ArrResponse::~ArrResponse() {
    for (Response *element : elements) {
        delete element;
    }
}

void ArrResponse::serialize(Buffer &buf) {
    buf.append_uint8(ResponseTag::TAG_ARR);
    buf.append_uint32(len);
    for (Response *element : elements) {
        element->serialize(buf);
    }
}

ArrResponse* ArrResponse::deserialize(const char *buf) {
    buf += 1; // skip tag

    uint32_t len;
    read_uint32(&len, &buf);

    std::vector<Response *> elements;
    for (uint32_t i = 0; i < len; i++) {
        uint8_t element_tag;
        read_uint8(&element_tag, &buf);
        buf -= 1; // undo increment by read_uint8()
        
        Response *element = NULL;
        switch (element_tag) {
            case ResponseTag::TAG_NIL:
                element = NilResponse::deserialize(buf);
                break;
            case ResponseTag::TAG_STR:
                element = StrResponse::deserialize(buf);
                break;
            case ResponseTag::TAG_ERR:
                element = ErrResponse::deserialize(buf);
                break;
            case ResponseTag::TAG_INT:
                element = IntResponse::deserialize(buf);
                break;
            case ResponseTag::TAG_ARR:
                element = ArrResponse::deserialize(buf);
                break;       
            case ResponseTag::TAG_DBL:
                element = DblResponse::deserialize(buf);
                break;     
            default:
                fatal("invalid element type at position %d in ArrResponse", i);    
        }

        elements.push_back(element);
        buf += element->length(); // move buf pointer to next element in array
    }

    return new ArrResponse(elements);
}

uint32_t ArrResponse::length() {
    uint32_t elements_size = 0;
    for (Response *element : elements) {
        elements_size += element->length();
    }

    return TAG_SIZE + LEN_SIZE + elements_size;
}

std::string ArrResponse::to_string() {
    std::string output = std::format("(array) len={}", len);
    for (Response *element : elements) {
        output += std::format("\n{}", element->to_string());
    }
    output += std::string("\n(array) end");
    return output;
}
