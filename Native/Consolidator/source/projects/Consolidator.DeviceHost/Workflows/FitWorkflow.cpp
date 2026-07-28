#include "FitWorkflow.h"

#include "DSP/Eq/EqRuntime.h"
#include "Settings/AudioOptions.h"

#include <algorithm>
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
    commitHidden = false;
    activeMode = domain::FitMode::Eq;
    Publish(domain::FitRequestedEvent{
        state.sessionId, activeBankId, command.curveDb
    });
}

void FitWorkflow::Handle(const domain::CommitHiddenEqBankCommand& command) {
    if (state.status == domain::ApproximatorState::Status::Processing) {
        Reject(command.requestId, "fit_in_progress");
        return;
    }
    if (command.bankId.value < models::EqSnapshot::FirstUserBankId ||
        command.bankId.value > models::EqSnapshot::LastUserBankId ||
        !eqStore.IsUserBankEmpty(static_cast<long>(command.bankId.value))) {
        Reject(command.requestId, "invalid_commit_hidden");
        return;
    }
    consolidator::dsp::EqRuntime runtime;
    runtime.SetSnapshot(eqStore.State());
    const auto hiddenCurve = runtime.BuildBankCurve(
        models::EqSnapshot::SystemBankId,
        consolidator::settings::AudioOptions::DefaultSampleRateHz);
    if (hiddenCurve.Values().empty() || std::all_of(hiddenCurve.Values().begin(), hiddenCurve.Values().end(),
        [](double value) { return std::abs(value) < 1.0e-12; })) {
        Reject(command.requestId, "empty_hidden_bank");
        return;
    }
    state.status = domain::ApproximatorState::Status::Processing;
    state.progress = 0.0;
    state.loss = 0.0;
    state.error.clear();
    ++state.sessionId.value;
    activeBankId = command.bankId;
    commitHidden = true;
    activeMode = domain::FitMode::Eq;
    Publish(domain::FitRequestedEvent{ state.sessionId, activeBankId, hiddenCurve.Values() });
}

void FitWorkflow::Handle(const domain::CancelFitCommand& command) {
    if (state.status != domain::ApproximatorState::Status::Processing ||
        state.sessionId != command.sessionId) {
        Reject(command.requestId, "stale_fit_session");
        return;
    }

    const auto operation = domain::FitOperationName(activeMode);
    state.status = domain::ApproximatorState::Status::Idle;
    activeBankId = {};
    commitHidden = false;
    Publish(domain::OperationChangedEvent{
        operation, command.sessionId, domain::OperationStatus::Cancelled, 0.0, {}
    });
}

void FitWorkflow::Handle(const domain::ClearFitCommand&) {
    const auto operation = domain::FitOperationName(activeMode);
    state.status = domain::ApproximatorState::Status::Idle;
    state.progress = 0.0;
    state.loss = 0.0;
    state.error.clear();
    activeBankId = {};
    commitHidden = false;
    Publish(domain::OperationChangedEvent{
        operation, state.sessionId, domain::OperationStatus::Idle, 0.0, {}
    });
}

void FitWorkflow::Handle(const domain::CompleteFitCommand& command) {
    if (state.status != domain::ApproximatorState::Status::Processing ||
        command.result.sessionId != state.sessionId ||
        command.result.bankId != activeBankId) {
        Reject(command.requestId, "stale_fit_result");
        return;
    }
    auto fitCommand = command;
    fitCommand.result.processor.compressor.outputDb = compressorStore.State().outputDb;
    fitCommand.result.processor.compressor.mix = compressorStore.State().mix;
    fitCommand.result.processor.compressor.detectorFilters = compressorStore.State().detectorFilters;
    fitCommand.result.processor.saturator.detectorFilters = saturatorStore.State().detectorFilters;
    if (!std::isfinite(fitCommand.result.loss)) {
        Fail(command.requestId, "invalid_fit_result", true);
        return;
    }
    if (!inputGainStore.CanApplyFit(fitCommand.result.processor.inputGain) ||
        !compressorStore.CanApplyFit(fitCommand.result.processor.compressor) ||
        !saturatorStore.CanApplyFit(fitCommand.result.processor.saturator) ||
        !outputGainStore.CanApplyFit(fitCommand.result.processor.outputGain)) {
        Fail(command.requestId, "invalid_fit_processor_state", true);
        return;
    }

    const auto result = commitHidden
        ? eqStore.ApplyCommitHiddenResult(fitCommand)
        : eqStore.ApplyFitResult(fitCommand);
    if (!result.Accepted()) {
        Fail(command.requestId, result.error, true);
        return;
    }
    const auto inputGainResult = inputGainStore.ApplyFit(
        fitCommand.result.processor.inputGain, command.requestId);
    const auto compressorResult = compressorStore.ApplyFit(
        fitCommand.result.processor.compressor, command.requestId);
    const auto saturatorResult = saturatorStore.ApplyFit(
        fitCommand.result.processor.saturator, command.requestId);
    const auto outputGainResult = outputGainStore.ApplyFit(
        fitCommand.result.processor.outputGain, command.requestId);
    if (!inputGainResult.Accepted() || !compressorResult.Accepted() ||
        !saturatorResult.Accepted() || !outputGainResult.Accepted()) {
        Fail(command.requestId, "invalid_fit_processor_state", true);
        return;
    }

    const auto operation = domain::FitOperationName(activeMode);
    state.status = domain::ApproximatorState::Status::Completed;
    state.progress = 1.0;
    state.loss = fitCommand.result.loss;
    state.error.clear();
    activeBankId = {};
    commitHidden = false;
    Publish(domain::OperationChangedEvent{
        operation, state.sessionId, domain::OperationStatus::Completed, 1.0, {}
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
    const auto operation = domain::FitOperationName(activeMode);
    state.status = domain::ApproximatorState::Status::Failed;
    state.error = error;
    activeBankId = {};
    commitHidden = false;
    if (rejectCommand) Reject(requestId, error);
    Publish(domain::OperationChangedEvent{
        operation, state.sessionId, domain::OperationStatus::Failed, state.progress, error
    });
}

void FitWorkflow::Publish(domain::Event event) const {
    if (eventHandler) eventHandler(std::move(event));
}

} // namespace consolidator::host
