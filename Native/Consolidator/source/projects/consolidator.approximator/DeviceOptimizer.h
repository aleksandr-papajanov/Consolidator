#pragma once

#include "FitAudioBuffer.h"
#include "Models/ParameterRange.h"
#include "OfflineFitEvaluator.h"
#include "Settings/CompressorOptions.h"
#include "Settings/FilterOptions.h"
#include "Settings/FitOptions.h"
#include "Settings/GainOptions.h"
#include "Settings/SaturatorOptions.h"
#include "Snapshots/Snapshots.h"

#include <nlopt.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

class DeviceOptimizer final {
public:
    struct Result final {
        consolidator::domain::DspSnapshot snapshot;
        double loss = 0.0;
    };

    Result Optimize(
        consolidator::domain::DspSnapshot snapshot,
        const FitAudioBuffer& audio,
        const std::atomic<bool>& cancelRequested
    ) const {
        auto bindings = BuildBindings(snapshot);
        if (bindings.empty()) throw std::runtime_error("no_optimizable_parameters");

        std::vector<double> values;
        values.reserve(bindings.size());
        for (const auto& binding : bindings) values.push_back(binding.ReadNormalized(snapshot));

        OfflineFitEvaluator evaluator;
        Context context{
            std::move(snapshot), &bindings, &audio, &cancelRequested, &evaluator
        };
        nlopt::opt optimizer{ nlopt::LN_BOBYQA, static_cast<unsigned>(values.size()) };
        optimizer.set_lower_bounds(std::vector<double>(values.size(), 0.0));
        optimizer.set_upper_bounds(std::vector<double>(values.size(), 1.0));
        optimizer.set_min_objective(&Evaluate, &context);
        optimizer.set_initial_step(consolidator::settings::FitOptions::InitialStep);
        optimizer.set_ftol_rel(consolidator::settings::FitOptions::RelativeFunctionTolerance);
        optimizer.set_xtol_rel(consolidator::settings::FitOptions::RelativeParameterTolerance);
        optimizer.set_maxeval(consolidator::settings::FitOptions::MaximumEvaluations);

        double loss = std::numeric_limits<double>::infinity();
        try {
            optimizer.optimize(values, loss);
        }
        catch (const nlopt::roundoff_limited&) {
            loss = context.bestLoss;
        }

        if (cancelRequested.load(std::memory_order_acquire)) {
            throw nlopt::forced_stop();
        }
        if (!std::isfinite(context.bestLoss)) throw std::runtime_error("optimizer_failed");
        return { std::move(context.bestSnapshot), context.bestLoss };
    }

private:
    enum class ParameterKind {
        Filter,
        InputGain,
        CompressorAttack,
        CompressorRelease,
        CompressorThreshold,
        Saturation,
        OutputGain
    };

    struct ParameterBinding final {
        ParameterKind kind{};
        std::size_t filterIndex = 0;
        std::size_t parameterIndex = 0;
        consolidator::models::ParameterRange range;

        double ReadNormalized(const consolidator::domain::DspSnapshot& snapshot) const {
            return std::clamp(range.Normalize(ReadAbsolute(snapshot)), 0.0, 1.0);
        }

        void WriteNormalized(
            consolidator::domain::DspSnapshot& snapshot,
            double normalized
        ) const {
            const auto value = range.Denormalize(std::clamp(normalized, 0.0, 1.0));
            auto* bank = snapshot.eq.SelectedBank();
            switch (kind) {
                case ParameterKind::Filter:
                    bank->filters[filterIndex].values[parameterIndex] = value;
                    break;
                case ParameterKind::InputGain:
                    snapshot.processor.inputGain.gainDb = value;
                    break;
                case ParameterKind::CompressorAttack:
                    snapshot.processor.compressor.attackMs = value;
                    break;
                case ParameterKind::CompressorRelease:
                    snapshot.processor.compressor.releaseMs = value;
                    break;
                case ParameterKind::CompressorThreshold:
                    snapshot.processor.compressor.thresholdDb = value;
                    break;
                case ParameterKind::Saturation:
                    snapshot.processor.saturator.saturation = value;
                    break;
                case ParameterKind::OutputGain:
                    snapshot.processor.outputGain.gainDb = value;
                    break;
            }
        }

    private:
        double ReadAbsolute(const consolidator::domain::DspSnapshot& snapshot) const {
            const auto* bank = snapshot.eq.SelectedBank();
            switch (kind) {
                case ParameterKind::Filter:
                    return bank->filters[filterIndex].values[parameterIndex];
                case ParameterKind::InputGain:
                    return snapshot.processor.inputGain.gainDb;
                case ParameterKind::CompressorAttack:
                    return snapshot.processor.compressor.attackMs;
                case ParameterKind::CompressorRelease:
                    return snapshot.processor.compressor.releaseMs;
                case ParameterKind::CompressorThreshold:
                    return snapshot.processor.compressor.thresholdDb;
                case ParameterKind::Saturation:
                    return snapshot.processor.saturator.saturation;
                case ParameterKind::OutputGain:
                    return snapshot.processor.outputGain.gainDb;
            }
            return 0.0;
        }
    };

    struct Context final {
        consolidator::domain::DspSnapshot baseSnapshot;
        const std::vector<ParameterBinding>* bindings = nullptr;
        const FitAudioBuffer* audio = nullptr;
        const std::atomic<bool>* cancelRequested = nullptr;
        OfflineFitEvaluator* evaluator = nullptr;
        consolidator::domain::DspSnapshot bestSnapshot;
        double bestLoss = std::numeric_limits<double>::infinity();
    };

    static double Evaluate(
        const std::vector<double>& values,
        std::vector<double>& gradient,
        void* contextPointer
    ) {
        (void)gradient;
        auto& context = *static_cast<Context*>(contextPointer);
        if (context.cancelRequested->load(std::memory_order_acquire)) {
            throw nlopt::forced_stop();
        }

        auto candidate = context.baseSnapshot;
        for (std::size_t index = 0; index < values.size(); ++index) {
            (*context.bindings)[index].WriteNormalized(candidate, values[index]);
        }
        const auto loss = context.evaluator->Evaluate(candidate, *context.audio);
        if (!loss || !std::isfinite(*loss)) return std::numeric_limits<double>::infinity();
        if (*loss < context.bestLoss) {
            context.bestLoss = *loss;
            context.bestSnapshot = candidate;
        }
        return *loss;
    }

    static std::vector<ParameterBinding> BuildBindings(
        const consolidator::domain::DspSnapshot& snapshot
    ) {
        using consolidator::models::ParameterRange;
        using consolidator::models::ParameterScale;
        std::vector<ParameterBinding> result;
        const auto* bank = snapshot.eq.SelectedBank();
        if (!bank) return result;

        const auto& definitions = consolidator::settings::FilterOptions::Definitions();
        for (std::size_t filterIndex = 0; filterIndex < bank->filters.size(); ++filterIndex) {
            const auto& filter = bank->filters[filterIndex];
            const auto definition = definitions.find(filter.filterId);
            if (definition == definitions.end() || filter.bypass ||
                (definition->second.section == consolidator::models::EqSection::Pre
                    ? bank->preBypass : bank->postBypass)) continue;
            for (std::size_t parameterIndex = 0;
                 parameterIndex < definition->second.parameters.size();
                 ++parameterIndex) {
                result.push_back({ ParameterKind::Filter, filterIndex, parameterIndex,
                    definition->second.parameters[parameterIndex].range });
            }
        }

        result.push_back({ ParameterKind::InputGain, 0, 0,
            ParameterRange{ consolidator::settings::GainOptions::MinimumGainDb,
                consolidator::settings::GainOptions::MaximumGainDb, ParameterScale::Linear } });
        if (!snapshot.processor.compressor.bypass) {
            result.push_back({ ParameterKind::CompressorAttack, 0, 0,
                ParameterRange{ consolidator::settings::CompressorOptions::MinimumAttackMs,
                    consolidator::settings::CompressorOptions::MaximumAttackMs,
                    ParameterScale::Logarithmic } });
            result.push_back({ ParameterKind::CompressorRelease, 0, 0,
                ParameterRange{ consolidator::settings::CompressorOptions::MinimumReleaseMs,
                    consolidator::settings::CompressorOptions::MaximumReleaseMs,
                    ParameterScale::Logarithmic } });
            result.push_back({ ParameterKind::CompressorThreshold, 0, 0,
                ParameterRange{ consolidator::settings::CompressorOptions::MinimumThresholdDb,
                    consolidator::settings::CompressorOptions::MaximumThresholdDb,
                    ParameterScale::Linear } });
        }
        if (!snapshot.processor.saturator.bypass) {
            result.push_back({ ParameterKind::Saturation, 0, 0,
                ParameterRange{ consolidator::settings::SaturatorOptions::MinimumSaturation,
                    consolidator::settings::SaturatorOptions::MaximumSaturation,
                    ParameterScale::Linear } });
        }
        result.push_back({ ParameterKind::OutputGain, 0, 0,
            ParameterRange{ consolidator::settings::GainOptions::MinimumGainDb,
                consolidator::settings::GainOptions::MaximumGainDb, ParameterScale::Linear } });
        return result;
    }
};
