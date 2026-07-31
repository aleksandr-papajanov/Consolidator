#include "FitWorkflow.h"

#include "DSP/Eq/EqRuntime.h"
#include "Settings/AudioOptions.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace consolidator::host {

FitWorkflow::FitWorkflow(
    EqStore& eqStore,
    EventHandler eventHandler
) : eqStore(eqStore), eventHandler(std::move(eventHandler)) {}

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
    commitAll = false;
    commitSourceRevision = 0;
    activeMode = domain::FitMode::Eq;
    Publish(domain::FitRequestedEvent{
        state.sessionId, activeBankId, domain::FitTargetKind::Residual, command.curveDb
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
    commitAll = false;
    commitSourceRevision = eqStore.Revision();
    activeMode = domain::FitMode::Eq;
    Publish(domain::FitRequestedEvent{
        state.sessionId, activeBankId, domain::FitTargetKind::Absolute, hiddenCurve.Values()
    });
}

void FitWorkflow::Handle(const domain::CommitAllEqBanksCommand& command) {
    if (state.status == domain::ApproximatorState::Status::Processing) {
        Reject(command.requestId, "fit_in_progress");
        return;
    }

    auto sourceState = eqStore.State();
    sourceState.solo = false;
    consolidator::dsp::EqRuntime runtime;
    runtime.SetSnapshot(std::move(sourceState));
    const auto fullCurve = runtime.BuildAllBanksCurve(
        consolidator::settings::AudioOptions::DefaultSampleRateHz);
    if (fullCurve.Values().empty() || std::all_of(fullCurve.Values().begin(), fullCurve.Values().end(),
        [](double value) { return std::abs(value) < 1.0e-12; })) {
        Reject(command.requestId, "empty_eq_chain");
        return;
    }

    state.status = domain::ApproximatorState::Status::Processing;
    state.progress = 0.0;
    state.loss = 0.0;
    state.error.clear();
    ++state.sessionId.value;
    activeBankId = { models::EqSnapshot::IndividualBankId };
    commitHidden = false;
    commitAll = true;
    commitSourceRevision = eqStore.Revision();
    activeMode = domain::FitMode::Eq;
    Publish(domain::FitRequestedEvent{
        state.sessionId, activeBankId, domain::FitTargetKind::Absolute, fullCurve.Values()
    });
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
    commitAll = false;
    commitSourceRevision = 0;
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
    commitAll = false;
    commitSourceRevision = 0;
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
    if (!std::isfinite(command.result.loss)) {
        Fail(command.requestId, "invalid_fit_result", true);
        return;
    }

    const auto result = commitAll
        ? eqStore.ApplyCommitAllResult(command, commitSourceRevision)
        : commitHidden
            ? eqStore.ApplyCommitHiddenResult(command)
            : eqStore.ApplyFitResult(command);
    if (!result.Accepted()) {
        Fail(command.requestId, result.error, true);
        return;
    }

    const auto operation = domain::FitOperationName(activeMode);
    state.status = domain::ApproximatorState::Status::Completed;
    state.progress = 1.0;
    state.loss = command.result.loss;
    state.error.clear();
    activeBankId = {};
    commitHidden = false;
    commitAll = false;
    commitSourceRevision = 0;
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
    commitAll = false;
    commitSourceRevision = 0;
    if (rejectCommand) Reject(requestId, error);
    Publish(domain::OperationChangedEvent{
        operation, state.sessionId, domain::OperationStatus::Failed, state.progress, error
    });
}

void FitWorkflow::Publish(domain::Event event) const {
    if (eventHandler) eventHandler(std::move(event));
}

} // namespace consolidator::host
