#pragma once

#include "../MessagePayload.h"

#include <string_view>

namespace consolidator::messaging {

class IMessage {
public:
    virtual ~IMessage() = default;

    virtual std::string_view Type() const = 0;
    virtual MessagePayload Serialize() const = 0;
};

} // namespace consolidator::messaging
