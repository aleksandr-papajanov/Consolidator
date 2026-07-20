#pragma once

#include "EqStore.h"
#include "Workflows/AnalyzerWorkflow.h"
#include "Workflows/FitWorkflow.h"
#include "Events/Events.h"
#include "Commands/Commands.h"
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <vector>

namespace consolidator::host {

class DeviceHost final {
public:
    using EventHandler = std::function<void(const domain::Event&)>;

    explicit DeviceHost(EventHandler eventHandler = {});

    void Handle(const domain::Command& command);
    const EqStore& Eq() const noexcept;
    domain::StoreRevision Revision() const noexcept;
    bool RestoreEq(domain::EqState state, domain::StoreRevision revision);

private:
    void HandleComponent(const domain::AttachComponentCommand& command);
    void HandleComponent(const domain::DetachComponentCommand& command);
    void PublishResult(const UpdateResult& result, domain::RequestId requestId);
    void Publish(domain::Event event);
    void Dispatch(const std::vector<domain::Event>& events) const;

    mutable std::mutex mutex;
    EqStore eqStore;
    AnalyzerWorkflow analyzerWorkflow;
    FitWorkflow fitWorkflow;
    EventHandler eventHandler;
    std::map<std::int64_t, std::string> components;
    std::vector<domain::Event>* activeEvents = nullptr;
};

} // namespace consolidator::host
