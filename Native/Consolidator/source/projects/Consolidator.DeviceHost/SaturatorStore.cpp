#include "SaturatorStore.h"

#include "Settings/DetectorFilterOptions.h"
#include "Settings/SaturatorOptions.h"

#include <cmath>
#include <cstddef>
#include <utility>

namespace consolidator::host {

SaturatorStore::SaturatorStore(CommitHandler commitHandler)
    : commitHandler(std::move(commitHandler)) {}

const domain::SaturatorState& SaturatorStore::State() const noexcept { return state; }
domain::StoreRevision SaturatorStore::Revision() const noexcept { return revision; }

UpdateResult SaturatorStore::SetParameter(const domain::SetSaturatorParameterCommand& command) {
    double* destination = nullptr;
    double minimum = 0.0;
    double maximum = 0.0;
    if (command.parameter == "input") {
        destination = &state.inputDb;
        minimum = settings::SaturatorOptions::MinimumInputDb;
        maximum = settings::SaturatorOptions::MaximumInputDb;
    } else if (command.parameter == "output") {
        destination = &state.outputDb;
        minimum = settings::SaturatorOptions::MinimumOutputDb;
        maximum = settings::SaturatorOptions::MaximumOutputDb;
    }
    if (!destination || !std::isfinite(command.value) || command.value < minimum || command.value > maximum) {
        return Reject("invalid_saturator_parameter");
    }
    if (*destination == command.value) return { UpdateStatus::Unchanged, {} };
    *destination = command.value;
    return Commit(command.requestId);
}

UpdateResult SaturatorStore::SetLink(const domain::SetProcessorLinkCommand& command) {
    if (state.linkId == command.linkId) return { UpdateStatus::Unchanged, {} };
    state.linkId = command.linkId;
    return Commit(command.requestId);
}

UpdateResult SaturatorStore::SetBypass(const domain::SetSaturatorBypassCommand& command) {
    if (state.bypass == command.bypass) return { UpdateStatus::Unchanged, {} };
    state.bypass = command.bypass;
    return Commit(command.requestId);
}

UpdateResult SaturatorStore::SetMode(const domain::SetSaturatorModeCommand& command) {
    if (command.mode < 0 || command.mode >= settings::SaturatorOptions::ModeCount) return Reject("invalid_saturator_mode");
    if (state.mode == command.mode) return { UpdateStatus::Unchanged, {} };
    state.mode = command.mode;
    return Commit(command.requestId);
}

UpdateResult SaturatorStore::SetDetectorParameter(const domain::SetSaturatorDetectorParameterCommand& command) {
    if (command.filterId < 1 || command.filterId > 2 || !std::isfinite(command.value)) return Reject("invalid_saturator_detector_filter");
    auto& filter = state.detectorFilters[static_cast<std::size_t>(command.filterId - 1)];
    filter.filterId = command.filterId;
    double* destination = nullptr;
    double minimum = 0.0;
    double maximum = 0.0;
    if (command.parameter == "bypass") {
        if (command.value != 0.0 && command.value != 1.0) return Reject("invalid_saturator_detector_bypass");
        const auto nextValue = command.value != 0.0;
        if (filter.bypass == nextValue) return { UpdateStatus::Unchanged, {} };
        filter.bypass = nextValue;
        return Commit(command.requestId);
    }
    if (command.parameter == "gain") destination = &filter.gainDb;
    else if (command.parameter == "frequency") destination = &filter.frequencyHz;
    else if (command.parameter == "q") destination = &filter.q;
    else return Reject("invalid_saturator_detector_parameter");
    const auto* definition = settings::DetectorFilterOptions::Definition().FindParameter(command.parameter);
    if (!definition) return Reject("invalid_saturator_detector_parameter");
    minimum = definition->range.minimum;
    maximum = definition->range.maximum;
    if (command.value < minimum || command.value > maximum) return Reject("saturator_detector_value_out_of_range");
    if (*destination == command.value) return { UpdateStatus::Unchanged, {} };
    *destination = command.value;
    return Commit(command.requestId);
}

UpdateResult SaturatorStore::SetDetectorListen(const domain::SetSaturatorDetectorListenCommand& command) {
    if (command.filterId < 0 || command.filterId > 2) return Reject("invalid_saturator_detector_listen");
    if (state.detectorListen == command.filterId) return { UpdateStatus::Unchanged, {} };
    state.detectorListen = command.filterId;
    return Commit(command.requestId);
}

UpdateResult SaturatorStore::Reset(const domain::ResetSaturatorCommand& command) {
    auto defaults = domain::SaturatorState{};
    defaults.linkId = state.linkId;
    bool detectorChanged = false;
    for (std::size_t index = 0; index < state.detectorFilters.size(); ++index) {
        const auto& current = state.detectorFilters[index];
        const auto& fallback = defaults.detectorFilters[index];
        detectorChanged = detectorChanged || current.bypass != fallback.bypass || current.gainDb != fallback.gainDb ||
            current.frequencyHz != fallback.frequencyHz || current.q != fallback.q;
    }
    if (!detectorChanged && state.inputDb == defaults.inputDb && state.outputDb == defaults.outputDb &&
        state.mode == defaults.mode && state.detectorListen == defaults.detectorListen &&
        state.bypass == defaults.bypass) {
        return { UpdateStatus::Unchanged, {} };
    }
    state = defaults;
    return Commit(command.requestId);
}

UpdateResult SaturatorStore::Replace(domain::SaturatorState nextState, domain::StoreRevision nextRevision) {
    if (!std::isfinite(nextState.inputDb) ||
        nextState.inputDb < settings::SaturatorOptions::MinimumInputDb ||
        nextState.inputDb > settings::SaturatorOptions::MaximumInputDb ||
        !std::isfinite(nextState.outputDb) ||
        nextState.outputDb < settings::SaturatorOptions::MinimumOutputDb ||
        nextState.outputDb > settings::SaturatorOptions::MaximumOutputDb ||
        nextState.mode < 0 || nextState.mode >= settings::SaturatorOptions::ModeCount) {
        return Reject("invalid_persisted_saturator_state");
    }
    state = nextState;
    revision = nextRevision;
    return { UpdateStatus::Changed, {} };
}

bool SaturatorStore::CanApplyFit(const domain::SaturatorState& nextState) const noexcept {
    return std::isfinite(nextState.inputDb) &&
        nextState.inputDb >= settings::SaturatorOptions::MinimumInputDb &&
        nextState.inputDb <= settings::SaturatorOptions::MaximumInputDb &&
        std::isfinite(nextState.outputDb) &&
        nextState.outputDb >= settings::SaturatorOptions::MinimumOutputDb &&
        nextState.outputDb <= settings::SaturatorOptions::MaximumOutputDb &&
        nextState.mode >= 0 &&
        nextState.mode < settings::SaturatorOptions::ModeCount;
}

UpdateResult SaturatorStore::ApplyFit(
    domain::SaturatorState nextState,
    domain::RequestId requestId
) {
    nextState.linkId = state.linkId;
    if (!CanApplyFit(nextState)) {
        return Reject("invalid_fit_saturator_state");
    }
    if (state.inputDb == nextState.inputDb && state.outputDb == nextState.outputDb &&
        state.mode == nextState.mode && state.bypass == nextState.bypass &&
        state.detectorFilters == nextState.detectorFilters) {
        return { UpdateStatus::Unchanged, {} };
    }
    state = nextState;
    return Commit(requestId);
}

UpdateResult SaturatorStore::Commit(domain::RequestId requestId) {
    ++revision;
    if (commitHandler) commitHandler(revision, requestId);
    return { UpdateStatus::Changed, {} };
}

UpdateResult SaturatorStore::Reject(const char* error) const { return { UpdateStatus::Rejected, error }; }

} // namespace consolidator::host
