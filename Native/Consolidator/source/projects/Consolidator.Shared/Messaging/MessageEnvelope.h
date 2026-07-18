#pragma once

#include <string>

#include "MessagePayload.h"

namespace consolidator::messaging {

// Transport-neutral metadata plus a structured serializable payload.
struct MessageEnvelope {
    std::string type;
    std::string source;
    std::string target;
    MessagePayload payload;
};

} // namespace consolidator::messaging
