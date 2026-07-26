#pragma once

#include "FitAudioBuffer.h"
#include "Models/ParameterRange.h"
#include "OfflineFitEvaluator.h"
#include "Settings/FilterOptions.h"
#include "Settings/FitOptions.h"
#include "Settings/CompressorOptions.h"
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
    enum class Scope {
        Eq,
        Dynamics,
        Saturation
    };

    struct Result final {
        consolidator::domain::DspSnapshot snapshot;
        double loss = 0.0;
        bool budgetExceeded = false;
    };

    Result Optimize(
        consolidator::domain::DspSnapshot snapshot,
        const FitAudioBuffer& audio,
        const std::atomic<bool>& cancelRequested,
        int maximumEvaluations = consolidator::settings::FitOptions::MaximumEvaluations,
        std::atomic<std::size_t>* sharedEvaluationBudget = nullptr,
        Scope scope = Scope::Eq
    ) const {
        PrepareScope(snapshot, scope);
        auto bindings = BuildBindings(snapshot, scope);
        if (bindings.empty()) throw std::runtime_error("no_optimizable_parameters");
        if (maximumEvaluations <= 0) throw std::invalid_argument("invalid_optimizer_budget");

        std::vector<double> values;
        values.reserve(bindings.size());
        for (const auto& binding : bindings) values.push_back(binding.ReadNormalized(snapshot));

        OfflineFitEvaluator evaluator;
        Context context{
            std::move(snapshot), &bindings, &audio, &cancelRequested, &evaluator,
            sharedEvaluationBudget
        };
        nlopt::opt optimizer{ nlopt::LN_BOBYQA, static_cast<unsigned>(values.size()) };
        optimizer.set_lower_bounds(std::vector<double>(values.size(), 0.0));
        optimizer.set_upper_bounds(std::vector<double>(values.size(), 1.0));
        optimizer.set_min_objective(&Evaluate, &context);
        optimizer.set_initial_step(consolidator::settings::FitOptions::InitialStep);
        optimizer.set_ftol_rel(consolidator::settings::FitOptions::RelativeFunctionTolerance);
        optimizer.set_xtol_rel(consolidator::settings::FitOptions::RelativeParameterTolerance);
        optimizer.set_maxeval(maximumEvaluations);

        double loss = std::numeric_limits<double>::infinity();
        try {
            optimizer.optimize(values, loss);
        }
        catch (const nlopt::roundoff_limited&) {
            loss = context.bestLoss;
        }
        catch (const nlopt::forced_stop&) {
            if (!std::isfinite(context.bestLoss)) throw;
        }

        if (cancelRequested.load(std::memory_order_acquire)) {
            throw nlopt::forced_stop();
        }
        if (!std::isfinite(context.bestLoss)) throw std::runtime_error("optimizer_failed");
        return {
            std::move(context.bestSnapshot),
            context.bestLoss,
            context.budgetExceeded
        };
    }

public:
    enum class ParameterKind {
        Filter,
        CompressorAttack,
        CompressorRelease,
        CompressorInput,
        SaturatorInput,
        SaturatorOutput
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
                case ParameterKind::CompressorAttack:
                    snapshot.processor.compressor.attackMs = value;
                    break;
                case ParameterKind::CompressorRelease:
                    snapshot.processor.compressor.releaseMs = value;
                    break;
                case ParameterKind::CompressorInput:
                    snapshot.processor.compressor.inputDb = value;
                    break;
                case ParameterKind::SaturatorInput:
                    snapshot.processor.saturator.inputDb = value;
                    break;
                case ParameterKind::SaturatorOutput:
                    snapshot.processor.saturator.outputDb = value;
                    break;
            }
        }

    private:
        double ReadAbsolute(const consolidator::domain::DspSnapshot& snapshot) const {
            const auto* bank = snapshot.eq.SelectedBank();
            switch (kind) {
                case ParameterKind::Filter:
                    return bank->filters[filterIndex].values[parameterIndex];
                case ParameterKind::CompressorAttack:
                    return snapshot.processor.compressor.attackMs;
                case ParameterKind::CompressorRelease:
                    return snapshot.processor.compressor.releaseMs;
                case ParameterKind::CompressorInput:
                    return snapshot.processor.compressor.inputDb;
                case ParameterKind::SaturatorInput:
                    return snapshot.processor.saturator.inputDb;
                case ParameterKind::SaturatorOutput:
                    return snapshot.processor.saturator.outputDb;
            }
            return 0.0;
        }
    };

private:
    struct Context final {
        consolidator::domain::DspSnapshot baseSnapshot;
        const std::vector<ParameterBinding>* bindings = nullptr;
        const FitAudioBuffer* audio = nullptr;
        const std::atomic<bool>* cancelRequested = nullptr;
        OfflineFitEvaluator* evaluator = nullptr;
        std::atomic<std::size_t>* sharedEvaluationBudget = nullptr;
        consolidator::domain::DspSnapshot bestSnapshot;
        double bestLoss = std::numeric_limits<double>::infinity();
        bool budgetExceeded = false;
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
        if (context.sharedEvaluationBudget) {
            auto remaining = context.sharedEvaluationBudget->load(std::memory_order_acquire);
            while (remaining > 0 &&
                   !context.sharedEvaluationBudget->compare_exchange_weak(
                       remaining, remaining - 1, std::memory_order_acq_rel,
                       std::memory_order_acquire)) {}
            if (remaining == 0) {
                context.budgetExceeded = true;
                throw nlopt::forced_stop();
            }
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

public:
    static std::vector<ParameterBinding> BuildBindings(
        const consolidator::domain::DspSnapshot& snapshot,
        Scope scope = Scope::Eq
    ) {
        using consolidator::models::ParameterRange;
        using consolidator::models::ParameterScale;
        std::vector<ParameterBinding> result;
        if (scope == Scope::Dynamics) {
            result.push_back({ ParameterKind::CompressorAttack, 0, 0,
                ParameterRange{ consolidator::settings::CompressorOptions::MinimumAttackMs,
                    consolidator::settings::CompressorOptions::MaximumAttackMs,
                    ParameterScale::Logarithmic } });
            result.push_back({ ParameterKind::CompressorRelease, 0, 0,
                ParameterRange{ consolidator::settings::CompressorOptions::MinimumReleaseMs,
                    consolidator::settings::CompressorOptions::MaximumReleaseMs,
                    ParameterScale::Logarithmic } });
            result.push_back({ ParameterKind::CompressorInput, 0, 0,
                ParameterRange{ consolidator::settings::CompressorOptions::MinimumInputDb,
                    consolidator::settings::CompressorOptions::MaximumInputDb,
                    ParameterScale::Linear } });
            return result;
        }
        if (scope == Scope::Saturation) {
            result.push_back({ ParameterKind::SaturatorInput, 0, 0,
                ParameterRange{ consolidator::settings::SaturatorOptions::MinimumInputDb,
                    consolidator::settings::SaturatorOptions::MaximumInputDb,
                    ParameterScale::Linear } });
            result.push_back({ ParameterKind::SaturatorOutput, 0, 0,
                ParameterRange{ consolidator::settings::SaturatorOptions::MinimumOutputDb,
                    consolidator::settings::SaturatorOptions::MaximumOutputDb,
                    ParameterScale::Linear } });
            return result;
        }

        const auto* bank = snapshot.eq.SelectedBank();
        if (!bank) return result;

        const auto& definitions = consolidator::settings::FilterOptions::EqDefinitions();
        for (std::size_t filterIndex = 0; filterIndex < bank->filters.size(); ++filterIndex) {
            const auto& filter = bank->filters[filterIndex];
            const auto definition = definitions.find(filter.filterId);
            if (definition == definitions.end() || filter.bypass || snapshot.eq.IsBypassed()) continue;
            for (std::size_t parameterIndex = 0;
                 parameterIndex < definition->second.parameters.size();
                 ++parameterIndex) {
                result.push_back({ ParameterKind::Filter, filterIndex, parameterIndex,
                    definition->second.parameters[parameterIndex].range });
            }
        }

        return result;
    }

private:
    static void PrepareScope(
        consolidator::domain::DspSnapshot& snapshot,
        Scope scope
    ) noexcept {
        if (scope == Scope::Dynamics) snapshot.processor.compressor.bypass = false;
        else if (scope == Scope::Saturation) snapshot.processor.saturator.bypass = false;
    }
};
