#include "CompressorStore.h"
#include "Settings/CompressorOptions.h"

#include <cmath>
#include <utility>

namespace consolidator::host {

CompressorStore::CompressorStore(CommitHandler commitHandler)
    : commitHandler(std::move(commitHandler)) {}

const domain::CompressorState& CompressorStore::State() const noexcept { return state; }
domain::StoreRevision CompressorStore::Revision() const noexcept { return revision; }

UpdateResult CompressorStore::SetParameter(const domain::SetCompressorParameterCommand& command) {
    if (!std::isfinite(command.value)) return Reject("invalid_compressor_value");
    double* destination = nullptr;
    double minimum = 0.0;
    double maximum = 0.0;
    if (command.parameter == "attack") {
        destination = &state.attackMs; minimum = settings::CompressorOptions::MinimumAttackMs; maximum = settings::CompressorOptions::MaximumAttackMs;
    }
    else if (command.parameter == "release") {
        destination = &state.releaseMs; minimum = settings::CompressorOptions::MinimumReleaseMs; maximum = settings::CompressorOptions::MaximumReleaseMs;
    }
    else if (command.parameter == "threshold") {
        destination = &state.thresholdDb; minimum = settings::CompressorOptions::MinimumThresholdDb; maximum = settings::CompressorOptions::MaximumThresholdDb;
    }
    else return Reject("invalid_compressor_parameter");
    if (command.value < minimum || command.value > maximum) return Reject("compressor_value_out_of_range");
    if (*destination == command.value) return { UpdateStatus::Unchanged, {} };
    *destination = command.value;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::SetBypass(const domain::SetCompressorBypassCommand& command) {
    if (state.bypass == command.bypass) return { UpdateStatus::Unchanged, {} };
    state.bypass = command.bypass;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::Reset(const domain::ResetCompressorCommand& command) {
    const domain::CompressorState defaults;
    if (state.attackMs == defaults.attackMs && state.releaseMs == defaults.releaseMs &&
        state.thresholdDb == defaults.thresholdDb && state.bypass == defaults.bypass) {
        return { UpdateStatus::Unchanged, {} };
    }
    state = defaults;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::Replace(domain::CompressorState nextState, domain::StoreRevision nextRevision) {
    if (!std::isfinite(nextState.attackMs) || nextState.attackMs < settings::CompressorOptions::MinimumAttackMs || nextState.attackMs > settings::CompressorOptions::MaximumAttackMs ||
        !std::isfinite(nextState.releaseMs) || nextState.releaseMs < settings::CompressorOptions::MinimumReleaseMs || nextState.releaseMs > settings::CompressorOptions::MaximumReleaseMs ||
        !std::isfinite(nextState.thresholdDb) || nextState.thresholdDb < settings::CompressorOptions::MinimumThresholdDb || nextState.thresholdDb > settings::CompressorOptions::MaximumThresholdDb) {
        return Reject("invalid_persisted_compressor_state");
    }
    state = nextState;
    revision = nextRevision;
    return { UpdateStatus::Changed, {} };
}

bool CompressorStore::CanApplyFit(const domain::CompressorState& nextState) const noexcept {
    return std::isfinite(nextState.attackMs) &&
        nextState.attackMs >= settings::CompressorOptions::MinimumAttackMs &&
        nextState.attackMs <= settings::CompressorOptions::MaximumAttackMs &&
        std::isfinite(nextState.releaseMs) &&
        nextState.releaseMs >= settings::CompressorOptions::MinimumReleaseMs &&
        nextState.releaseMs <= settings::CompressorOptions::MaximumReleaseMs &&
        std::isfinite(nextState.thresholdDb) &&
        nextState.thresholdDb >= settings::CompressorOptions::MinimumThresholdDb &&
        nextState.thresholdDb <= settings::CompressorOptions::MaximumThresholdDb;
}

UpdateResult CompressorStore::ApplyFit(
    domain::CompressorState nextState,
    domain::RequestId requestId
) {
    if (!CanApplyFit(nextState)) {
        return Reject("invalid_fit_compressor_state");
    }
    if (state.attackMs == nextState.attackMs && state.releaseMs == nextState.releaseMs &&
        state.thresholdDb == nextState.thresholdDb && state.bypass == nextState.bypass) {
        return { UpdateStatus::Unchanged, {} };
    }
    state = nextState;
    return Commit(requestId);
}

UpdateResult CompressorStore::Commit(domain::RequestId requestId) {
    ++revision;
    if (commitHandler) commitHandler(revision, requestId);
    return { UpdateStatus::Changed, {} };
}

UpdateResult CompressorStore::Reject(const char* error) const { return { UpdateStatus::Rejected, error }; }

} // namespace consolidator::host
