#pragma once

#include "Ids/DomainIds.h"
#include "Commands/Commands.h"
#include "Models/EqSnapshot.h"
#include "Models/ProcessorState.h"

#include <map>
#include <mutex>
#include <optional>
#include <cstddef>
#include <string>
#include <vector>

namespace consolidator::host {

class DeviceHost;

struct LinkCoordinatorEntry final {
    std::string runtimeId;
    std::string trackName;
    long trackOrder = 0;
    domain::StoreRevision revision = 0;
    models::EqSnapshot eq;
    models::ProcessorState processor;
};

struct LinkCoordinatorMember final {
    std::string runtimeId;
    long bankId = 0;
};

struct LinkedFilterGesture final {
    std::string linkId;
    std::string sourceRuntimeId;
    long filterId = 0;
    std::size_t parameterIndex = 0;
    double sourceNormalized = 0.0;
    double targetNormalized = 0.0;
};

struct LinkedProcessorGesture final {
    std::string linkId;
    std::string sourceRuntimeId;
    std::string device;
    std::string parameter;
    double sourceNormalized = 0.0;
    double targetNormalized = 0.0;
};

struct RoutedCommand final {
    std::string sourceRuntimeId;
    long bankId = 0;
    domain::Command command;
};

// Process-local runtime registry. Dispatch converts normalized gestures to
// absolute commands using cached host state, then calls host->Handle().
class LinkCoordinator final {
public:
    static LinkCoordinator& Instance();

    void Upsert(LinkCoordinatorEntry entry);
    void RegisterHost(const std::string& runtimeId, DeviceHost& host);
    void Remove(const std::string& runtimeId);
    void UpdateState(
        const std::string& runtimeId,
        domain::StoreRevision revision,
        const models::EqSnapshot& eq,
        const models::ProcessorState& processor);

    std::vector<LinkCoordinatorEntry> Entries() const;
    std::optional<LinkCoordinatorEntry> Find(const std::string& runtimeId) const;
    std::vector<LinkCoordinatorMember> Members(const std::string& linkId) const;

    void Dispatch(const LinkedFilterGesture& gesture);
    void Dispatch(const LinkedProcessorGesture& gesture);
    void DispatchCommandToHost(const std::string& runtimeId, const domain::Command& command) const;
    void DispatchCommandToGroup(const std::string& linkId, const domain::Command& command) const;
    void DispatchCommandToAll(const domain::Command& command) const;

    static std::optional<double> ReadProcessorValue(
        const models::ProcessorState& processor,
        const std::string& device,
        const std::string& parameter);

private:
    void AddLinksFor(const std::string& runtimeId, const models::EqSnapshot& eq);
    void RemoveLinksFor(const std::string& runtimeId);

    mutable std::mutex mutex;
    std::map<std::string, LinkCoordinatorEntry, std::less<>> entries;
    std::map<std::string, std::vector<std::string>, std::less<>> linkMembers;
    std::map<std::string, DeviceHost*, std::less<>> hosts;
};

} // namespace consolidator::host