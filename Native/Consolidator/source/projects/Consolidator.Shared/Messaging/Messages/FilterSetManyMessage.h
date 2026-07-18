#pragma once

#include "FilterStateMessage.h"
#include "../../Models/FilterState.h"

namespace consolidator::messaging {

class FilterSetManyMessage final : public FilterStateMessage {
public:
    static constexpr const char* TypeName = "filter.set_many";

    explicit FilterSetManyMessage(models::FilterState state)
        : state(std::move(state)) {}

    std::string_view Type() const override {
        return TypeName;
    }

    MessagePayload Serialize() const override {
        return SerializeState(state);
    }

    static std::optional<FilterSetManyMessage> Deserialize(const MessagePayload& payload) {
        const auto state = DeserializeState(payload);
        return state ? std::optional<FilterSetManyMessage>{ *state } : std::nullopt;
    }

    models::FilterState state;
};

} // namespace consolidator::messaging
