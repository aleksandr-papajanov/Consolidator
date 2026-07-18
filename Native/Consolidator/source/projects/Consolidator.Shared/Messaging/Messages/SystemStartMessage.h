#pragma once

#include "IMessage.h"

namespace consolidator::messaging {

class SystemStartMessage final : public IMessage {
public:
    static constexpr const char* TypeName = "system.start";

    std::string_view Type() const override {
        return TypeName;
    }

    MessagePayload Serialize() const override {
        return {};
    }

    static std::optional<SystemStartMessage> Deserialize(const MessagePayload&) {
        return SystemStartMessage{};
    }
};

} // namespace consolidator::messaging
