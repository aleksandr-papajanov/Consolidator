#pragma once

#include "Commands/Commands.h"
#include "EqStore.h"
#include "GainStore.h"
#include "CompressorStore.h"
#include "SaturatorStore.h"
#include "Events/Events.h"
#include "States/States.h"

#include <functional>
#include <string>

namespace consolidator::host {

class FitWorkflow final {
public:
    using EventHandler = std::function<void(domain::Event)>;

    FitWorkflow(
        EqStore& eqStore,
        GainStore& inputGainStore,
        CompressorStore& compressorStore,
        SaturatorStore& saturatorStore,
        GainStore& outputGainStore,
        EventHandler eventHandler = {});

    void Handle(const domain::StartFitCommand& command);
    void Handle(const domain::CommitHiddenEqBankCommand& command);
    void Handle(const domain::CancelFitCommand& command);
    void Handle(const domain::ClearFitCommand& command);
    void Handle(const domain::CompleteFitCommand& command);
    void Handle(const domain::FailFitCommand& command);
    const domain::ApproximatorState& State() const noexcept;

private:
    void Reject(domain::RequestId requestId, const std::string& error);
    void Fail(domain::RequestId requestId, const std::string& error, bool rejectCommand);
    void Publish(domain::Event event) const;

    EqStore& eqStore;
    GainStore& inputGainStore;
    CompressorStore& compressorStore;
    SaturatorStore& saturatorStore;
    GainStore& outputGainStore;
    domain::ApproximatorState state;
    domain::BankId activeBankId{};
    domain::FitMode activeMode = domain::FitMode::Eq;
    bool commitHidden = false;
    EventHandler eventHandler;
};

} // namespace consolidator::host
