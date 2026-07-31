#include "CompressorStore.h"
#include <cmath>
#include <cstddef>
#include <utility>

namespace consolidator::host {

CompressorStore::CompressorStore(CommitHandler commitHandler)
    : commitHandler(std::move(commitHandler)) {}

const domain::CompressorState& CompressorStore::State() const noexcept { return state; }
domain::StoreRevision CompressorStore::Revision() const noexcept { return revision; }

UpdateResult CompressorStore::SetParameter(const domain::SetCompressorParameterCommand& command) {
    if (!std::isfinite(command.value)) return Reject("invalid_compressor_value");
    double* destination = nullptr;
    if (command.parameter == "attack") {
        destination = &state.attackMs;
    }
    else if (command.parameter == "release") {
        destination = &state.releaseMs;
    }
    else if (command.parameter == "threshold") {
        destination = &state.thresholdDb;
    }
    else if (command.parameter == "output") {
        destination = &state.outputDb;
    }
    else if (command.parameter == "mix") {
        destination = &state.mix;
    }
    else return Reject("invalid_compressor_parameter");
    if (*destination == command.value) return { UpdateStatus::Unchanged, {} };
    *destination = command.value;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::SetDetectorParameter(const domain::SetCompressorDetectorParameterCommand& command) {
    if (command.filterId < 1 || command.filterId > 2 || !std::isfinite(command.value)) return Reject("invalid_compressor_detector_filter");
    auto& filter = state.detectorFilters[static_cast<std::size_t>(command.filterId - 1)];
    filter.filterId = command.filterId;
    double* destination = nullptr;
    if (command.parameter == "bypass") {
        if (command.value != 0.0 && command.value != 1.0) return Reject("invalid_compressor_detector_bypass");
        const auto nextValue = command.value != 0.0;
        if (filter.bypass == nextValue) return { UpdateStatus::Unchanged, {} };
        filter.bypass = nextValue;
        if (nextValue) state.detectorListen &= ~(1L << (command.filterId - 1));
        return Commit(command.requestId);
    }
    if (command.parameter == "gain") destination = &filter.gainDb;
    else if (command.parameter == "frequency") destination = &filter.frequencyHz;
    else if (command.parameter == "q") destination = &filter.q;
    else return Reject("invalid_compressor_detector_parameter");
    if (*destination == command.value) return { UpdateStatus::Unchanged, {} };
    *destination = command.value;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::SetDetectorListen(const domain::SetCompressorDetectorListenCommand& command) {
    if (command.filterId < 1 || command.filterId > 2) return Reject("invalid_compressor_detector_listen");
    if (state.detectorFilters[static_cast<std::size_t>(command.filterId - 1)].bypass) return { UpdateStatus::Unchanged, {} };
    const auto mask = 1L << (command.filterId - 1);
    const auto nextListen = command.enabled ? state.detectorListen | mask : state.detectorListen & ~mask;
    if (state.detectorListen == nextListen) return { UpdateStatus::Unchanged, {} };
    state.detectorListen = nextListen;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::SetBypass(const domain::SetCompressorBypassCommand& command) {
    if (state.bypass == command.bypass) return { UpdateStatus::Unchanged, {} };
    state.bypass = command.bypass;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::Reset(const domain::ResetCompressorCommand& command) {
    auto defaults = domain::CompressorState{};
    bool detectorChanged = false;
    for (std::size_t index = 0; index < state.detectorFilters.size(); ++index) {
        const auto& current = state.detectorFilters[index];
        const auto& fallback = defaults.detectorFilters[index];
        detectorChanged = detectorChanged || current.bypass != fallback.bypass || current.gainDb != fallback.gainDb ||
            current.frequencyHz != fallback.frequencyHz || current.q != fallback.q;
    }
    if (!detectorChanged && state.attackMs == defaults.attackMs && state.releaseMs == defaults.releaseMs &&
        state.thresholdDb == defaults.thresholdDb && state.outputDb == defaults.outputDb &&
        state.mix == defaults.mix &&
        state.detectorListen == defaults.detectorListen && state.bypass == defaults.bypass) {
        return { UpdateStatus::Unchanged, {} };
    }
    state = defaults;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::Replace(domain::CompressorState nextState, domain::StoreRevision nextRevision) {
    if (!std::isfinite(nextState.attackMs) || !std::isfinite(nextState.releaseMs) ||
        !std::isfinite(nextState.thresholdDb) || !std::isfinite(nextState.outputDb) ||
        !std::isfinite(nextState.mix)) {
        return Reject("invalid_persisted_compressor_state");
    }
    state = nextState;
    revision = nextRevision;
    return { UpdateStatus::Changed, {} };
}

bool CompressorStore::CanApplyFit(const domain::CompressorState& nextState) const noexcept {
    return std::isfinite(nextState.attackMs) && std::isfinite(nextState.releaseMs) &&
        std::isfinite(nextState.thresholdDb) && std::isfinite(nextState.outputDb) &&
        std::isfinite(nextState.mix);
}

UpdateResult CompressorStore::ApplyFit(
    domain::CompressorState nextState,
    domain::RequestId requestId
) {
    if (!CanApplyFit(nextState)) {
        return Reject("invalid_fit_compressor_state");
    }
    if (state.attackMs == nextState.attackMs && state.releaseMs == nextState.releaseMs &&
        state.thresholdDb == nextState.thresholdDb && state.outputDb == nextState.outputDb && state.mix == nextState.mix &&
        state.bypass == nextState.bypass &&
        state.detectorFilters == nextState.detectorFilters) {
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
