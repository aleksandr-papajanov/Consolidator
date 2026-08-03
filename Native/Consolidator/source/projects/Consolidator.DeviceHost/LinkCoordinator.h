#pragma once

#include "Ids/DomainIds.h"
#include "Commands/Commands.h"
#include "Models/EqSnapshot.h"
#include "Models/ProcessorState.h"

#include <map>
#include <mutex>
#include <optional>
#include <functional>
#include <cstddef>
#include <string>
#include <vector>

namespace consolidator::host {

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

struct LinkCoordinatorCallbacks final {
    std::function<void(const LinkedFilterGesture&)> applyFilter;
    std::function<void(const LinkedProcessorGesture&)> applyProcessor;
    std::function<void(const RoutedCommand&)> applyCommand;
};

// Process-local runtime registry. DeviceHost remains the owner of persisted state.
class LinkCoordinator final {
public:
    static LinkCoordinator& Instance();

    void Upsert(LinkCoordinatorEntry entry);
    void RegisterCallbacks(const std::string& runtimeId, LinkCoordinatorCallbacks callbacks);
    void Remove(const std::string& runtimeId);
    void UpdateState(
        const std::string& runtimeId,
        domain::StoreRevision revision,
        const models::EqSnapshot& eq,
        const models::ProcessorState& processor);

    std::vector<LinkCoordinatorEntry> Entries() const;
    std::optional<LinkCoordinatorEntry> Find(const std::string& runtimeId) const;
    std::vector<LinkCoordinatorMember> Members(const std::string& linkId) const;
    void Dispatch(const LinkedFilterGesture& gesture) const;
    void Dispatch(const LinkedProcessorGesture& gesture) const;
    void DispatchCommand(const std::string& runtimeId, RoutedCommand command) const;

private:
    mutable std::mutex mutex;
    std::map<std::string, LinkCoordinatorEntry, std::less<>> entries;
    std::map<std::string, LinkCoordinatorCallbacks, std::less<>> callbacks;
};

} // namespace consolidator::host
