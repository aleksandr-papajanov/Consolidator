#pragma once

#include "IMessage.h"

#include <optional>
#include <string>
#include <utility>

namespace consolidator::messaging {

class DeviceStateChangedMessage final : public IMessage {
public:
    static constexpr const char* TypeName = "device.state.changed";

    DeviceStateChangedMessage(std::string stateName, long generation)
        : stateName(std::move(stateName)), generation(generation) {}

    std::string_view Type() const override {
        return TypeName;
    }

    MessagePayload Serialize() const override {
        MessagePayload payload;
        payload.Set("stateName", stateName);
        payload.Set("generation", static_cast<std::int64_t>(generation));
        return payload;
    }

    static std::optional<DeviceStateChangedMessage> Deserialize(const MessagePayload& payload) {
        const auto stateName = payload.ReadString("stateName");
        const auto generation = payload.ReadLong("generation");
        if (!stateName || !generation) return std::nullopt;
        return DeviceStateChangedMessage{ *stateName, *generation };
    }

    std::string stateName;
    long generation = 0;
};

} // namespace consolidator::messaging
