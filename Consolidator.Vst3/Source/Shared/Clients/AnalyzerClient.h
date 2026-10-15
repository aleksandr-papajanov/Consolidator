#pragma once

#include <cstdint>
#include <optional>

#include "Shared/Clients/AnalyzerData.h"
#include "Shared/Clients/ProtocolClient.h"
#include "Shared/Clients/TargetStateClient.h"

namespace consolidator::vst3::shared::clients
{

class AnalyzerClient final
{
public:
    AnalyzerClient(shared::clients::ProtocolClient& commands, const shared::clients::TargetStateClient& state);
    void Invalidate();
    void FetchBanks();
    [[nodiscard]] std::optional<shared::clients::BankSnapshots> HandleBanks(const shared::protocol::ProtocolFrame& frame);
    [[nodiscard]] std::optional<shared::clients::AnalyzerSpectrum> HandleSpectrum(const shared::protocol::ProtocolFrame& frame) const;

private:
    shared::clients::ProtocolClient& commands_;
    const shared::clients::TargetStateClient& state_;
    std::uint64_t banksRequest_{};
    bool banksDirty_{};
};

} // namespace consolidator::vst3::shared::clients
