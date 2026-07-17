#pragma once

#include "MessageEnvelope.h"

namespace consolidator::protocol {

// The transport factory creates the common envelope. Domain handlers decide
// how to interpret payload fields for their registered protocol type.
class MessageFactory {
public:
    static std::unique_ptr<MessageEnvelope> from_atom(const c74::min::atom& value) {
        return MessageEnvelope::from_atom(value);
    }

    static bool matches(
        const MessageEnvelope& message,
        const char* type,
        const long expected_target = -1
    ) {
        std::string actual_type;
        if (!message.type(actual_type) || actual_type != type) {
            return false;
        }
        if (expected_target < 0) {
            return true;
        }
        long target = 0;
        return message.target(target) && target == expected_target;
    }
};

} // namespace consolidator::protocol
