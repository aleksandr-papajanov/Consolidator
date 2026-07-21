#pragma once

#include "DSP/Eq/EqRuntime.h"
#include "Snapshots/Snapshots.h"
#include "Settings/AudioOptions.h"

#include <optional>

namespace consolidator::dspcore {

class DspSnapshotBuilder final {
public:
    domain::DspSnapshot Build(
        const domain::EqState& eq,
        const domain::ProcessorState& processor,
        domain::StoreRevision revision
    ) const {
        return { revision, eq, processor };
    }
};

class AnalyzerSnapshotBuilder final {
public:
    explicit AnalyzerSnapshotBuilder(double sampleRate = settings::AudioOptions::DefaultSampleRateHz)
        : sampleRate(sampleRate) {}

    domain::AnalyzerSnapshot Build(const domain::EqState& state, domain::StoreRevision revision) const {
        dsp::EqRuntime runtime;
        runtime.SetSnapshot(state);
        domain::AnalyzerSnapshot result;
        result.revision = revision;
        result.eq = state;
        result.selectedBankCurve = runtime.BuildBankCurve(state.selectedBankId, sampleRate).Values();
        result.selectedPrefixCurve = runtime.BuildThroughBankCurve(state.selectedBankId, sampleRate).Values();
        result.totalCurve = runtime.BuildAllBanksCurve(sampleRate).Values();
        return result;
    }

private:
    double sampleRate;
};

class FitInputBuilder final {
public:
    explicit FitInputBuilder(double sampleRate = settings::AudioOptions::DefaultSampleRateHz)
        : sampleRate(sampleRate) {}

    std::optional<domain::FitInputSnapshot> Build(
        const domain::EqState& state,
        domain::StoreRevision revision
    ) const {
        const auto bank = state.FindBank(state.selectedBankId);
        if (!bank) return std::nullopt;
        dsp::EqRuntime runtime;
        runtime.SetSnapshot(state);
        return domain::FitInputSnapshot{
            revision,
            { state.selectedBankId },
            *bank,
            domain::FilterDefinitions(),
            runtime.BuildBankCurve(state.selectedBankId, sampleRate).Values()
        };
    }

private:
    double sampleRate;
};

} // namespace consolidator::dspcore
