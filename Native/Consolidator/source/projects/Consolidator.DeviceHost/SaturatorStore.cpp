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
    if (command.parameter == "saturation") {
        destination = &state.saturation;
        minimum = settings::SaturatorOptions::MinimumSaturation;
        maximum = settings::SaturatorOptions::MaximumSaturation;
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

UpdateResult SaturatorStore::SetBypass(const domain::SetSaturatorBypassCommand& command) {
    if (state.bypass == command.bypass) return { UpdateStatus::Unchanged, {} };
    state.bypass = command.bypass;
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
        if (nextValue) state.detectorListen &= ~(1L << (command.filterId - 1));
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
    if (command.filterId < 1 || command.filterId > 2) return Reject("invalid_saturator_detector_listen");
    if (state.detectorFilters[static_cast<std::size_t>(command.filterId - 1)].bypass) return { UpdateStatus::Unchanged, {} };
    const auto mask = 1L << (command.filterId - 1);
    const auto nextListen = command.enabled ? state.detectorListen | mask : state.detectorListen & ~mask;
    if (state.detectorListen == nextListen) return { UpdateStatus::Unchanged, {} };
    state.detectorListen = nextListen;
    return Commit(command.requestId);
}

UpdateResult SaturatorStore::Reset(const domain::ResetSaturatorCommand& command) {
    auto defaults = domain::SaturatorState{};
    bool detectorChanged = false;
    for (std::size_t index = 0; index < state.detectorFilters.size(); ++index) {
        const auto& current = state.detectorFilters[index];
        const auto& fallback = defaults.detectorFilters[index];
        detectorChanged = detectorChanged || current.bypass != fallback.bypass || current.gainDb != fallback.gainDb ||
            current.frequencyHz != fallback.frequencyHz || current.q != fallback.q;
    }
    if (!detectorChanged && state.saturation == defaults.saturation && state.outputDb == defaults.outputDb &&
        state.detectorListen == defaults.detectorListen &&
        state.bypass == defaults.bypass) {
        return { UpdateStatus::Unchanged, {} };
    }
    state = defaults;
    return Commit(command.requestId);
}

UpdateResult SaturatorStore::Replace(domain::SaturatorState nextState, domain::StoreRevision nextRevision) {
    if (!std::isfinite(nextState.saturation) ||
        nextState.saturation < settings::SaturatorOptions::MinimumSaturation ||
        nextState.saturation > settings::SaturatorOptions::MaximumSaturation ||
        !std::isfinite(nextState.outputDb) ||
        nextState.outputDb < settings::SaturatorOptions::MinimumOutputDb ||
        nextState.outputDb > settings::SaturatorOptions::MaximumOutputDb) {
        return Reject("invalid_persisted_saturator_state");
    }
    state = nextState;
    revision = nextRevision;
    return { UpdateStatus::Changed, {} };
}

bool SaturatorStore::CanApplyFit(const domain::SaturatorState& nextState) const noexcept {
    return std::isfinite(nextState.saturation) &&
        nextState.saturation >= settings::SaturatorOptions::MinimumSaturation &&
        nextState.saturation <= settings::SaturatorOptions::MaximumSaturation &&
        std::isfinite(nextState.outputDb) &&
        nextState.outputDb >= settings::SaturatorOptions::MinimumOutputDb &&
        nextState.outputDb <= settings::SaturatorOptions::MaximumOutputDb;
}

UpdateResult SaturatorStore::ApplyFit(
    domain::SaturatorState nextState,
    domain::RequestId requestId
) {
    if (!CanApplyFit(nextState)) {
        return Reject("invalid_fit_saturator_state");
    }
    if (state.saturation == nextState.saturation && state.outputDb == nextState.outputDb &&
        state.bypass == nextState.bypass &&
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
