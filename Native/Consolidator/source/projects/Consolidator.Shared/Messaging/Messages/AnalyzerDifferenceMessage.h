#pragma once

#include "IMessage.h"

namespace consolidator::messaging {

class AnalyzerDifferenceMessage final : public IMessage {
public:
    static constexpr const char* TypeName = "analyzer.difference";

    explicit AnalyzerDifferenceMessage(bool enabled)
        : enabled(enabled) {}

    std::string_view Type() const override {
        return TypeName;
    }

    MessagePayload Serialize() const override {
        MessagePayload payload;
        payload.Set("value", static_cast<std::int64_t>(enabled ? 1 : 0));
        return payload;
    }

    static std::optional<AnalyzerDifferenceMessage> Deserialize(const MessagePayload& payload) {
        const auto enabled = payload.ReadBool("value");
        return enabled ? std::optional<AnalyzerDifferenceMessage>{ *enabled } : std::nullopt;
    }

    bool enabled = false;
};

} // namespace consolidator::messaging
