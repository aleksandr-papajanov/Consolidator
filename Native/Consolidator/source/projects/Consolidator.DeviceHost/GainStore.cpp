#include "GainStore.h"
#include "Settings/GainOptions.h"

#include <cmath>
#include <utility>

namespace consolidator::host {

GainStore::GainStore(CommitHandler commitHandler)
    : commitHandler(std::move(commitHandler)) {}

const domain::GainState& GainStore::State() const noexcept { return state; }
domain::StoreRevision GainStore::Revision() const noexcept { return revision; }

UpdateResult GainStore::SetParameter(const domain::SetGainParameterCommand& command) {
    if (!std::isfinite(command.gainDb)) return Reject("invalid_gain_value");
    if (command.gainDb < settings::GainOptions::MinimumGainDb ||
        command.gainDb > settings::GainOptions::MaximumGainDb) {
        return Reject("gain_value_out_of_range");
    }
    if (state.gainDb == command.gainDb) return { UpdateStatus::Unchanged, {} };
    state.gainDb = command.gainDb;
    return Commit(command.requestId);
}

UpdateResult GainStore::SetLink(const domain::SetProcessorLinkCommand& command) {
    if (state.linkId == command.linkId) return { UpdateStatus::Unchanged, {} };
    state.linkId = command.linkId;
    return Commit(command.requestId);
}

UpdateResult GainStore::Replace(domain::GainState nextState, domain::StoreRevision nextRevision) {
    if (!std::isfinite(nextState.gainDb) ||
        nextState.gainDb < settings::GainOptions::MinimumGainDb ||
        nextState.gainDb > settings::GainOptions::MaximumGainDb) {
        return Reject("invalid_persisted_gain_state");
    }
    state = nextState;
    revision = nextRevision;
    return { UpdateStatus::Changed, {} };
}

bool GainStore::CanApplyFit(const domain::GainState& nextState) const noexcept {
    return std::isfinite(nextState.gainDb) &&
        nextState.gainDb >= settings::GainOptions::MinimumGainDb &&
        nextState.gainDb <= settings::GainOptions::MaximumGainDb;
}

UpdateResult GainStore::ApplyFit(domain::GainState nextState, domain::RequestId requestId) {
    if (!CanApplyFit(nextState)) {
        return Reject("invalid_fit_gain_state");
    }
    nextState.linkId = state.linkId;
    if (state.gainDb == nextState.gainDb) return { UpdateStatus::Unchanged, {} };
    state = nextState;
    return Commit(requestId);
}

UpdateResult GainStore::Commit(domain::RequestId requestId) {
    ++revision;
    if (commitHandler) commitHandler(revision, requestId);
    return { UpdateStatus::Changed, {} };
}

UpdateResult GainStore::Reject(const char* error) const {
    return { UpdateStatus::Rejected, error };
}

} // namespace consolidator::host
