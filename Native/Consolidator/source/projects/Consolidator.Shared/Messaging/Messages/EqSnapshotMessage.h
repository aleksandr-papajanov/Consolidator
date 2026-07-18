#pragma once

#include "IMessage.h"

#include <string>
#include <utility>

namespace consolidator::messaging {

class EqSnapshotMessage final : public IMessage {
public:
    static constexpr const char* TypeName = "eq.storage.snapshot";

    EqSnapshotMessage(std::string snapshotName, long selectedBankId)
        : snapshotName(std::move(snapshotName)), selectedBankId(selectedBankId) {}

    std::string_view Type() const override {
        return TypeName;
    }

    MessagePayload Serialize() const override {
        MessagePayload payload;
        payload.Set("snapshotName", snapshotName);
        payload.Set("selectedBankId", static_cast<std::int64_t>(selectedBankId));
        return payload;
    }

    static std::optional<EqSnapshotMessage> Deserialize(const MessagePayload& payload) {
        const auto snapshotName = payload.ReadString("snapshotName");
        const auto selectedBankId = payload.ReadLong("selectedBankId");
        return snapshotName && selectedBankId
            ? std::optional<EqSnapshotMessage>{ std::in_place, *snapshotName, *selectedBankId }
            : std::nullopt;
    }

    std::string snapshotName;
    long selectedBankId;
};

} // namespace consolidator::messaging
