#include "GainStore.h"
#include <cmath>
#include <utility>

namespace consolidator::host {

GainStore::GainStore(CommitHandler commitHandler)
    : commitHandler(std::move(commitHandler)) {}

const domain::GainState& GainStore::State() const noexcept { return state; }
domain::StoreRevision GainStore::Revision() const noexcept { return revision; }

UpdateResult GainStore::SetParameter(const domain::SetGainParameterCommand& command) {
    if (!std::isfinite(command.gainDb)) return Reject("invalid_gain_value");
    if (state.gainDb == command.gainDb) return { UpdateStatus::Unchanged, {} };
    state.gainDb = command.gainDb;
    return Commit(command.requestId);
}

UpdateResult GainStore::Replace(domain::GainState nextState, domain::StoreRevision nextRevision) {
    if (!std::isfinite(nextState.gainDb)) {
        return Reject("invalid_persisted_gain_state");
    }
    state = nextState;
    revision = nextRevision;
    return { UpdateStatus::Changed, {} };
}

bool GainStore::CanApplyFit(const domain::GainState& nextState) const noexcept {
    return std::isfinite(nextState.gainDb);
}

UpdateResult GainStore::ApplyFit(domain::GainState nextState, domain::RequestId requestId) {
    if (!CanApplyFit(nextState)) {
        return Reject("invalid_fit_gain_state");
    }
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
