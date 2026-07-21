#include "SaturatorStore.h"

#include "Settings/SaturatorOptions.h"

#include <cmath>
#include <utility>

namespace consolidator::host {

SaturatorStore::SaturatorStore(CommitHandler commitHandler)
    : commitHandler(std::move(commitHandler)) {}

const domain::SaturatorState& SaturatorStore::State() const noexcept { return state; }
domain::StoreRevision SaturatorStore::Revision() const noexcept { return revision; }

UpdateResult SaturatorStore::SetParameter(const domain::SetSaturatorParameterCommand& command) {
    if (!std::isfinite(command.saturation) || command.saturation < settings::SaturatorOptions::MinimumSaturation || command.saturation > settings::SaturatorOptions::MaximumSaturation) {
        return Reject("invalid_saturation");
    }
    if (state.saturation == command.saturation) return { UpdateStatus::Unchanged, {} };
    state.saturation = command.saturation;
    return Commit(command.requestId);
}

UpdateResult SaturatorStore::SetBypass(const domain::SetSaturatorBypassCommand& command) {
    if (state.bypass == command.bypass) return { UpdateStatus::Unchanged, {} };
    state.bypass = command.bypass;
    return Commit(command.requestId);
}

UpdateResult SaturatorStore::Reset(const domain::ResetSaturatorCommand& command) {
    const domain::SaturatorState defaults;
    if (state.saturation == defaults.saturation && state.bypass == defaults.bypass) {
        return { UpdateStatus::Unchanged, {} };
    }
    state = defaults;
    return Commit(command.requestId);
}

UpdateResult SaturatorStore::Replace(domain::SaturatorState nextState, domain::StoreRevision nextRevision) {
    if (!std::isfinite(nextState.saturation) || nextState.saturation < settings::SaturatorOptions::MinimumSaturation || nextState.saturation > settings::SaturatorOptions::MaximumSaturation) {
        return Reject("invalid_persisted_saturator_state");
    }
    state = nextState;
    revision = nextRevision;
    return { UpdateStatus::Changed, {} };
}

UpdateResult SaturatorStore::Commit(domain::RequestId requestId) {
    ++revision;
    if (commitHandler) commitHandler(revision, requestId);
    return { UpdateStatus::Changed, {} };
}

UpdateResult SaturatorStore::Reject(const char* error) const { return { UpdateStatus::Rejected, error }; }

} // namespace consolidator::host
