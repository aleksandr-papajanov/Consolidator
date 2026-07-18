#pragma once

#include "IMessage.h"

#include <optional>

namespace consolidator::messaging {

class FilterEditMessage final : public IMessage {
public:
    static constexpr const char* TypeName = "filter.edit";

    FilterEditMessage(
        long filterId,
        std::optional<double> frequencyHz,
        std::optional<double> gainDb,
        std::optional<double> normalizedQ
    )
        : filterId(filterId),
          frequencyHz(frequencyHz),
          gainDb(gainDb),
          normalizedQ(normalizedQ) {}

    std::string_view Type() const override {
        return TypeName;
    }

    MessagePayload Serialize() const override {
        MessagePayload payload;
        payload.Set("filterId", static_cast<std::int64_t>(filterId));
        if (frequencyHz) payload.Set("frequencyHz", *frequencyHz);
        if (gainDb) payload.Set("gainDb", *gainDb);
        if (normalizedQ) payload.Set("normalizedQ", *normalizedQ);
        return payload;
    }

    static std::optional<FilterEditMessage> Deserialize(const MessagePayload& payload) {
        const auto filterId = payload.ReadLong("filterId");
        return filterId
            ? std::optional<FilterEditMessage>{
                std::in_place,
                *filterId,
                payload.ReadDouble("frequencyHz"),
                payload.ReadDouble("gainDb"),
                payload.ReadDouble("normalizedQ")
            }
            : std::nullopt;
    }

    long filterId = 0;
    std::optional<double> frequencyHz;
    std::optional<double> gainDb;
    std::optional<double> normalizedQ;
};

} // namespace consolidator::messaging
