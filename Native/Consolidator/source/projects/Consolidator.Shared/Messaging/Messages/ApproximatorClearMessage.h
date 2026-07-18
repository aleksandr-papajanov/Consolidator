#pragma once

#include "IMessage.h"

namespace consolidator::messaging {

class ApproximatorClearMessage final : public IMessage {
public:
    static constexpr const char* TypeName = "approximator.clear";

    std::string_view Type() const override {
        return TypeName;
    }

    MessagePayload Serialize() const override {
        return {};
    }

    static std::optional<ApproximatorClearMessage> Deserialize(const MessagePayload&) {
        return ApproximatorClearMessage{};
    }
};

} // namespace consolidator::messaging
