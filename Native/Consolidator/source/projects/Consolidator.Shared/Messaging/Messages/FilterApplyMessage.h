#pragma once

#include "FilterStateMessage.h"
#include "../../Models/FilterState.h"

namespace consolidator::messaging {

class FilterApplyMessage final : public FilterStateMessage {
public:
    static constexpr const char* TypeName = "filter.apply";

    explicit FilterApplyMessage(models::FilterState state)
        : state(std::move(state)) {}

    std::string_view Type() const override {
        return TypeName;
    }

    MessagePayload Serialize() const override {
        return SerializeState(state);
    }

    static std::optional<FilterApplyMessage> Deserialize(const MessagePayload& payload) {
        const auto state = DeserializeState(payload);
        return state ? std::optional<FilterApplyMessage>{ *state } : std::nullopt;
    }

    models::FilterState state;
};

} // namespace consolidator::messaging
