#include "FitWorkflow.h"

#include <cmath>
#include <utility>

namespace consolidator::host {

FitWorkflow::FitWorkflow(
    EqStore& eqStore,
    GainStore& inputGainStore,
    CompressorStore& compressorStore,
    SaturatorStore& saturatorStore,
    GainStore& outputGainStore,
    EventHandler eventHandler
) : eqStore(eqStore), inputGainStore(inputGainStore), compressorStore(compressorStore),
    saturatorStore(saturatorStore), outputGainStore(outputGainStore),
    eventHandler(std::move(eventHandler)) {}

void FitWorkflow::Handle(const domain::StartFitCommand& command) {
    if (state.status == domain::ApproximatorState::Status::Processing) {
        Reject(command.requestId, "fit_in_progress");
        return;
    }

    state.status = domain::ApproximatorState::Status::Processing;
    state.progress = 0.0;
    state.loss = 0.0;
    state.error.clear();
    ++state.sessionId.value;
    activeBankId = { eqStore.State().selectedBankId };
    Publish(domain::OperationChangedEvent{
        "fit", state.sessionId, domain::OperationStatus::Starting, 0.0, {}
    });
}

void FitWorkflow::Handle(const domain::CancelFitCommand& command) {
    if (state.status != domain::ApproximatorState::Status::Processing ||
        state.sessionId != command.sessionId) {
        Reject(command.requestId, "stale_fit_session");
        return;
    }

    state.status = domain::ApproximatorState::Status::Idle;
    activeBankId = {};
    Publish(domain::OperationChangedEvent{
        "fit", command.sessionId, domain::OperationStatus::Cancelled, 0.0, {}
    });
}

void FitWorkflow::Handle(const domain::ClearFitCommand&) {
    state.status = domain::ApproximatorState::Status::Idle;
    state.progress = 0.0;
    state.loss = 0.0;
    state.error.clear();
    activeBankId = {};
    Publish(domain::OperationChangedEvent{
        "fit", state.sessionId, domain::OperationStatus::Idle, 0.0, {}
    });
}

void FitWorkflow::Handle(const domain::CompleteFitCommand& command) {
    if (state.status != domain::ApproximatorState::Status::Processing ||
        command.result.sessionId != state.sessionId ||
        command.result.bankId != activeBankId) {
        Reject(command.requestId, "stale_fit_result");
        return;
    }
    if (!std::isfinite(command.result.loss)) {
        Fail(command.requestId, "invalid_fit_result", true);
        return;
    }
    if (!inputGainStore.CanApplyFit(command.result.processor.inputGain) ||
        !compressorStore.CanApplyFit(command.result.processor.compressor) ||
        !saturatorStore.CanApplyFit(command.result.processor.saturator) ||
        !outputGainStore.CanApplyFit(command.result.processor.outputGain)) {
        Fail(command.requestId, "invalid_fit_processor_state", true);
        return;
    }

    const auto result = eqStore.ApplyFitResult(command);
    if (!result.Accepted()) {
        Fail(command.requestId, result.error, true);
        return;
    }
    const auto inputGainResult = inputGainStore.ApplyFit(
        command.result.processor.inputGain, command.requestId);
    const auto compressorResult = compressorStore.ApplyFit(
        command.result.processor.compressor, command.requestId);
    const auto saturatorResult = saturatorStore.ApplyFit(
        command.result.processor.saturator, command.requestId);
    const auto outputGainResult = outputGainStore.ApplyFit(
        command.result.processor.outputGain, command.requestId);
    if (!inputGainResult.Accepted() || !compressorResult.Accepted() ||
        !saturatorResult.Accepted() || !outputGainResult.Accepted()) {
        Fail(command.requestId, "invalid_fit_processor_state", true);
        return;
    }

    state.status = domain::ApproximatorState::Status::Completed;
    state.progress = 1.0;
    state.loss = command.result.loss;
    state.error.clear();
    activeBankId = {};
    Publish(domain::OperationChangedEvent{
        "fit", state.sessionId, domain::OperationStatus::Completed, 1.0, {}
    });
}

void FitWorkflow::Handle(const domain::FailFitCommand& command) {
    if (state.status != domain::ApproximatorState::Status::Processing ||
        command.sessionId != state.sessionId) {
        Reject(command.requestId, "stale_fit_failure");
        return;
    }

    Fail(command.requestId, command.error, false);
}

const domain::ApproximatorState& FitWorkflow::State() const noexcept {
    return state;
}

void FitWorkflow::Reject(domain::RequestId requestId, const std::string& error) {
    Publish(domain::CommandRejectedEvent{ requestId, error });
}

void FitWorkflow::Fail(
    domain::RequestId requestId,
    const std::string& error,
    bool rejectCommand
) {
    state.status = domain::ApproximatorState::Status::Failed;
    state.error = error;
    activeBankId = {};
    if (rejectCommand) Reject(requestId, error);
    Publish(domain::OperationChangedEvent{
        "fit", state.sessionId, domain::OperationStatus::Failed, state.progress, error
    });
}

void FitWorkflow::Publish(domain::Event event) const {
    if (eventHandler) eventHandler(std::move(event));
}

} // namespace consolidator::host
