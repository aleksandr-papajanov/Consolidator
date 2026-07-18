#pragma once

#include "IMessage.h"

namespace consolidator::messaging {

class ApproximatorFitMessage final : public IMessage {
public:
    static constexpr const char* TypeName = "approximator.fit";

    std::string_view Type() const override {
        return TypeName;
    }

    MessagePayload Serialize() const override {
        return {};
    }

    static std::optional<ApproximatorFitMessage> Deserialize(const MessagePayload&) {
        return ApproximatorFitMessage{};
    }
};

} // namespace consolidator::messaging
