#pragma once

#include "IMessage.h"

#include <optional>
#include <string>

namespace consolidator::messaging {

class EqBankChangedMessage final : public IMessage {
public:
    static constexpr const char* TypeName = "eq.storage.bank.changed";

    EqBankChangedMessage(std::string action, long bankIndex, std::string bankName)
        : action(std::move(action)), bankIndex(bankIndex), bankName(std::move(bankName)) {}

    std::string_view Type() const override {
        return TypeName;
    }

    MessagePayload Serialize() const override {
        MessagePayload payload;
        payload.Set("action", action);
        payload.Set("bankIndex", static_cast<std::int64_t>(bankIndex));
        payload.Set("bankName", bankName);
        if (filterId) payload.Set("filterId", static_cast<std::int64_t>(*filterId));
        return payload;
    }

    static std::optional<EqBankChangedMessage> Deserialize(const MessagePayload& payload) {
        const auto action = payload.ReadString("action");
        const auto bankIndex = payload.ReadLong("bankIndex");
        const auto bankName = payload.ReadString("bankName");
        if (!action || !bankIndex || !bankName) return std::nullopt;
        EqBankChangedMessage message{ *action, *bankIndex, *bankName };
        message.filterId = payload.ReadLong("filterId");
        return message;
    }

    std::string action;
    long bankIndex = 0;
    std::string bankName;
    std::optional<long> filterId;
};

} // namespace consolidator::messaging
