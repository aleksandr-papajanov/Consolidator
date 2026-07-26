#include "CompressorStore.h"
#include "Settings/DetectorFilterOptions.h"
#include "Settings/CompressorOptions.h"

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
    double minimum = 0.0;
    double maximum = 0.0;
    if (command.parameter == "attack") {
        destination = &state.attackMs; minimum = settings::CompressorOptions::MinimumAttackMs; maximum = settings::CompressorOptions::MaximumAttackMs;
    }
    else if (command.parameter == "release") {
        destination = &state.releaseMs; minimum = settings::CompressorOptions::MinimumReleaseMs; maximum = settings::CompressorOptions::MaximumReleaseMs;
    }
    else if (command.parameter == "input") {
        destination = &state.inputDb; minimum = settings::CompressorOptions::MinimumInputDb; maximum = settings::CompressorOptions::MaximumInputDb;
    }
    else if (command.parameter == "output") {
        destination = &state.outputDb; minimum = settings::CompressorOptions::MinimumOutputDb; maximum = settings::CompressorOptions::MaximumOutputDb;
    }
    else if (command.parameter == "mix") {
        destination = &state.mix; minimum = settings::CompressorOptions::MinimumMix; maximum = settings::CompressorOptions::MaximumMix;
    }
    else return Reject("invalid_compressor_parameter");
    if (command.value < minimum || command.value > maximum) return Reject("compressor_value_out_of_range");
    if (*destination == command.value) return { UpdateStatus::Unchanged, {} };
    *destination = command.value;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::SetLink(const domain::SetProcessorLinkCommand& command) {
    if (state.linkId == command.linkId) return { UpdateStatus::Unchanged, {} };
    state.linkId = command.linkId;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::SetMode(const domain::SetCompressorModeCommand& command) {
    if (command.mode < 0 || command.mode >= settings::CompressorOptions::ModeCount) return Reject("invalid_compressor_mode");
    if (state.mode == command.mode) return { UpdateStatus::Unchanged, {} };
    state.mode = command.mode;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::SetDetectorParameter(const domain::SetCompressorDetectorParameterCommand& command) {
    if (command.filterId < 1 || command.filterId > 2 || !std::isfinite(command.value)) return Reject("invalid_compressor_detector_filter");
    auto& filter = state.detectorFilters[static_cast<std::size_t>(command.filterId - 1)];
    filter.filterId = command.filterId;
    double* destination = nullptr;
    double minimum = 0.0;
    double maximum = 0.0;
    if (command.parameter == "bypass") {
        if (command.value != 0.0 && command.value != 1.0) return Reject("invalid_compressor_detector_bypass");
        const auto nextValue = command.value != 0.0;
        if (filter.bypass == nextValue) return { UpdateStatus::Unchanged, {} };
        filter.bypass = nextValue;
        return Commit(command.requestId);
    }
    if (command.parameter == "gain") destination = &filter.gainDb;
    else if (command.parameter == "frequency") destination = &filter.frequencyHz;
    else if (command.parameter == "q") destination = &filter.q;
    else return Reject("invalid_compressor_detector_parameter");
    const auto* definition = settings::DetectorFilterOptions::Definition().FindParameter(command.parameter);
    if (!definition) return Reject("invalid_compressor_detector_parameter");
    minimum = definition->range.minimum;
    maximum = definition->range.maximum;
    if (command.value < minimum || command.value > maximum) return Reject("compressor_detector_value_out_of_range");
    if (*destination == command.value) return { UpdateStatus::Unchanged, {} };
    *destination = command.value;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::SetDetectorListen(const domain::SetCompressorDetectorListenCommand& command) {
    if (command.filterId < 0 || command.filterId > 2) return Reject("invalid_compressor_detector_listen");
    if (state.detectorListen == command.filterId) return { UpdateStatus::Unchanged, {} };
    state.detectorListen = command.filterId;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::SetBypass(const domain::SetCompressorBypassCommand& command) {
    if (state.bypass == command.bypass) return { UpdateStatus::Unchanged, {} };
    state.bypass = command.bypass;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::Reset(const domain::ResetCompressorCommand& command) {
    auto defaults = domain::CompressorState{};
    defaults.linkId = state.linkId;
    bool detectorChanged = false;
    for (std::size_t index = 0; index < state.detectorFilters.size(); ++index) {
        const auto& current = state.detectorFilters[index];
        const auto& fallback = defaults.detectorFilters[index];
        detectorChanged = detectorChanged || current.bypass != fallback.bypass || current.gainDb != fallback.gainDb ||
            current.frequencyHz != fallback.frequencyHz || current.q != fallback.q;
    }
    if (!detectorChanged && state.attackMs == defaults.attackMs && state.releaseMs == defaults.releaseMs &&
        state.inputDb == defaults.inputDb && state.outputDb == defaults.outputDb &&
        state.mix == defaults.mix && state.mode == defaults.mode &&
        state.detectorListen == defaults.detectorListen && state.bypass == defaults.bypass) {
        return { UpdateStatus::Unchanged, {} };
    }
    state = defaults;
    return Commit(command.requestId);
}

UpdateResult CompressorStore::Replace(domain::CompressorState nextState, domain::StoreRevision nextRevision) {
    if (!std::isfinite(nextState.attackMs) || nextState.attackMs < settings::CompressorOptions::MinimumAttackMs || nextState.attackMs > settings::CompressorOptions::MaximumAttackMs ||
        !std::isfinite(nextState.releaseMs) || nextState.releaseMs < settings::CompressorOptions::MinimumReleaseMs || nextState.releaseMs > settings::CompressorOptions::MaximumReleaseMs ||
        !std::isfinite(nextState.inputDb) || nextState.inputDb < settings::CompressorOptions::MinimumInputDb || nextState.inputDb > settings::CompressorOptions::MaximumInputDb ||
        !std::isfinite(nextState.outputDb) || nextState.outputDb < settings::CompressorOptions::MinimumOutputDb || nextState.outputDb > settings::CompressorOptions::MaximumOutputDb ||
        !std::isfinite(nextState.mix) || nextState.mix < settings::CompressorOptions::MinimumMix || nextState.mix > settings::CompressorOptions::MaximumMix ||
        nextState.mode < 0 || nextState.mode >= settings::CompressorOptions::ModeCount) {
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
        std::isfinite(nextState.inputDb) && nextState.inputDb >= settings::CompressorOptions::MinimumInputDb && nextState.inputDb <= settings::CompressorOptions::MaximumInputDb &&
        std::isfinite(nextState.outputDb) && nextState.outputDb >= settings::CompressorOptions::MinimumOutputDb && nextState.outputDb <= settings::CompressorOptions::MaximumOutputDb &&
        std::isfinite(nextState.mix) && nextState.mix >= settings::CompressorOptions::MinimumMix && nextState.mix <= settings::CompressorOptions::MaximumMix &&
        nextState.mode >= 0 && nextState.mode < settings::CompressorOptions::ModeCount;
}

UpdateResult CompressorStore::ApplyFit(
    domain::CompressorState nextState,
    domain::RequestId requestId
) {
    nextState.linkId = state.linkId;
    if (!CanApplyFit(nextState)) {
        return Reject("invalid_fit_compressor_state");
    }
    if (state.attackMs == nextState.attackMs && state.releaseMs == nextState.releaseMs &&
        state.inputDb == nextState.inputDb && state.outputDb == nextState.outputDb && state.mix == nextState.mix &&
        state.mode == nextState.mode && state.bypass == nextState.bypass &&
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
