#pragma once

#include <cstdint>
#include <optional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "Shared/Clients/ParameterEntry.h"
#include "Shared/Clients/ProtocolClient.h"

namespace consolidator::vst3::shared::clients
{

struct UiTarget
{
    std::uint64_t instance;
    int bank;
};

class TargetStateClient final
{
public:
    explicit TargetStateClient(ProtocolClient& commands);
    void Observe(UiTarget target);
    void Invalidate();
    [[nodiscard]] bool HandleSnapshot(const shared::protocol::ProtocolFrame& frame);
    [[nodiscard]] std::optional<shared::clients::ParameterEntry> HandleChanged(const shared::protocol::ProtocolFrame& frame);
    [[nodiscard]] bool IsSnapshotRequest(std::uint64_t request) const;
    [[nodiscard]] bool Ready() const;
    [[nodiscard]] std::uint64_t Generation() const;
    [[nodiscard]] const UiTarget& Target() const;
    [[nodiscard]] const std::vector<shared::clients::ParameterEntry>& Values() const;
    [[nodiscard]] const shared::clients::ParameterEntry& Require(const std::string& path) const;
    [[nodiscard]] std::string WriteMode(const std::string& path, bool groupScope) const;

private:
    ProtocolClient& commands_;
    UiTarget target_{};
    std::map<std::uint64_t, std::string> snapshotRequests_;
    std::set<std::string> acceptedContexts_;
    std::vector<ParameterEntry> stagedValues_;
    std::vector<shared::clients::ParameterEntry> values_;
    bool ready_{};
    std::uint64_t generation_{};
};

} // namespace consolidator::vst3::shared::clients
