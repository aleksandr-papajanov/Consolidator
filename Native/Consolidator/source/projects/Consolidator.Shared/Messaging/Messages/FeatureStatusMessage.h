#pragma once

#include "IMessage.h"

#include <string>

namespace consolidator::messaging {

class FeatureStatusMessage final : public IMessage {
public:
    static constexpr const char* TypeName = "system.status";

    FeatureStatusMessage(std::string feature, std::string state)
        : feature(std::move(feature)), state(std::move(state)) {}

    std::string_view Type() const override {
        return TypeName;
    }

    MessagePayload Serialize() const override {
        MessagePayload payload;
        payload.Set("feature", feature);
        payload.Set("state", state);
        return payload;
    }

    static std::optional<FeatureStatusMessage> Deserialize(const MessagePayload& payload) {
        const auto feature = payload.ReadString("feature");
        const auto state = payload.ReadString("state");
        return feature && state
            ? std::optional<FeatureStatusMessage>{ std::in_place, *feature, *state }
            : std::nullopt;
    }

    std::string feature;
    std::string state;
};

} // namespace consolidator::messaging
