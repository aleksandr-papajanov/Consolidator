#pragma once

#include "EqStore.h"
#include "GainStore.h"
#include "CompressorStore.h"
#include "SaturatorStore.h"
#include "DeviceHistory.h"
#include "Workflows/AnalyzerWorkflow.h"
#include "Workflows/FitWorkflow.h"
#include "Events/Events.h"
#include "Commands/Commands.h"
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace consolidator::host {

class DeviceHost final {
public:
    using EventHandler = std::function<void(const domain::Event&)>;

    explicit DeviceHost(EventHandler eventHandler = {});

    void Handle(const domain::Command& command);
    const EqStore& Eq() const noexcept;
    const GainStore& InputGain() const noexcept;
    const GainStore& OutputGain() const noexcept;
    const CompressorStore& Compressor() const noexcept;
    const SaturatorStore& Saturator() const noexcept;
    domain::StoreRevision Revision() const noexcept;
    bool Restore(
        domain::EqState eq,
        domain::ProcessorState processor,
        domain::StoreRevision revision,
        std::string instanceId);
    const std::string& InstanceId() const noexcept;
    const std::string& LastRestoreError() const noexcept;

private:
    DeviceHistoryState CaptureHistoryState() const;
    std::string ActiveLinkId() const;
    bool RestoreHistoryState(DeviceHistoryState state, domain::RequestId requestId);
    static bool IsUndoableCommand(const domain::Command& command);
    void PublishHistoryState();
    void PublishResult(const UpdateResult& result, domain::RequestId requestId);
    void Publish(domain::Event event);
    void Dispatch(const std::vector<domain::Event>& events) const;

    mutable std::mutex mutex;
    EqStore eqStore;
    GainStore inputGainStore;
    CompressorStore compressorStore;
    SaturatorStore saturatorStore;
    GainStore outputGainStore;
    DeviceHistory history;
    AnalyzerWorkflow analyzerWorkflow;
    FitWorkflow fitWorkflow;
    EventHandler eventHandler;
    std::string instanceId;
    std::string lastRestoreError;
    std::vector<domain::Event>* activeEvents = nullptr;
};

} // namespace consolidator::host
