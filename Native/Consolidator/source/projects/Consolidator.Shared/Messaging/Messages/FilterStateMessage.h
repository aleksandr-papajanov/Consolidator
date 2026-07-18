#pragma once

#include "IMessage.h"
#include "../../Models/FilterState.h"

namespace consolidator::messaging {

class FilterStateMessage : public IMessage {
protected:
    static MessagePayload SerializeState(const models::FilterState& state) {
        MessagePayload payload;
        payload.Set("filterId", static_cast<std::int64_t>(state.filterId));
        MessageArray values;
        values.reserve(state.values.size());
        for (const double value : state.values) values.emplace_back(value);
        payload.Set("values", std::move(values));
        payload.Set("bypass", state.bypass);
        if (state.bankIndex) payload.Set("bankIndex", static_cast<std::int64_t>(*state.bankIndex));
        return payload;
    }

    static std::optional<models::FilterState> DeserializeState(const MessagePayload& payload) {
        const auto filterId = payload.ReadLong("filterId");
        const auto values = payload.ReadArray("values");
        const auto bypass = payload.ReadBool("bypass");
        if (!filterId || !values || !bypass) return std::nullopt;

        models::FilterState state;
        state.filterId = *filterId;
        state.bypass = *bypass;
        state.bankIndex = payload.ReadLong("bankIndex");
        state.values.reserve(values->size());
        for (const auto& value : *values) {
            if (const auto number = value.As<double>()) state.values.push_back(*number);
            else if (const auto integer = value.As<std::int64_t>()) {
                state.values.push_back(static_cast<double>(*integer));
            }
            else return std::nullopt;
        }
        return state;
    }
};

} // namespace consolidator::messaging
