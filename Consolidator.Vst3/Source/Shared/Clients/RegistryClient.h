#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Shared/Protocol/ProtocolFrame.h"

namespace consolidator::vst3::shared::clients
{

struct RegistryProcessor
{
    std::string id;
    bool active{};
    bool marker{};
    bool bypass{};
};

struct RegistryBank
{
    int index{};
    std::optional<std::uint32_t> group;
    bool active{};
    bool bypass{};
};

struct RegistryInstance
{
    std::uint64_t id{};
    std::string label;
    bool mute{};
    bool solo{};
    bool bypass{};
    std::vector<RegistryProcessor> processors;
    std::vector<RegistryBank> banks;
};

enum class RegistryUpdate
{
    Ignored,
    Changed,
    PresentationChanged,
    Resynchronize
};

// Presentation cache, ported from Max RegistryClient/RegistryDeltaApplier.
// Managed remains the only topology authority.
class RegistryClient final
{
public:
    void StartSnapshot(std::uint64_t request);
    void Invalidate();
    void CancelSnapshot(std::uint64_t request);
    [[nodiscard]] RegistryUpdate Handle(const shared::protocol::ProtocolFrame& frame);
    [[nodiscard]] bool Ready() const;
    [[nodiscard]] bool FetchPending() const;
    [[nodiscard]] const std::vector<RegistryInstance>& Instances() const;
    [[nodiscard]] const RegistryInstance* Find(std::uint64_t instance) const;
    [[nodiscard]] const RegistryBank* FindBank(std::uint64_t instance, int bank) const;
    [[nodiscard]] bool CanCreateGroup() const;

private:
    struct Snapshot
    {
        std::uint64_t request{};
        std::int64_t revision{};
        std::size_t expectedInstances{};
        std::size_t expectedGroups{};
        std::size_t groupsReceived{};
        bool began{};
        std::vector<RegistryInstance> instances;
    };

    void ApplyDelta(const shared::protocol::ProtocolFrame& frame);
    void ApplyMarkers(const shared::protocol::ProtocolFrame& frame);

    std::vector<RegistryInstance> instances_;
    std::optional<Snapshot> snapshot_;
    std::int64_t revision_{};
    std::int64_t requiredRevision_{};
    bool ready_{};
};

} // namespace consolidator::vst3::shared::clients
