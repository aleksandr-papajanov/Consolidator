#pragma once

#include "DSP/Curve/Curve.h"
#include "DSP/Eq/EqFilterFactory.h"
#include "Settings/AudioOptions.h"
#include "Settings/FilterOptions.h"
#include "Snapshots/Snapshots.h"

#include <nlopt.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

class EqCurveOptimizer final {
public:
    struct Result final {
        consolidator::domain::DspSnapshot snapshot;
        double loss = std::numeric_limits<double>::infinity();
    };

    Result Fit(
        const consolidator::dsp::Curve& target,
        consolidator::domain::DspSnapshot snapshot,
        const std::atomic<bool>& cancelRequested,
        double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz
    ) const {
        auto bindings = BuildBindings(snapshot);
        if (bindings.empty()) throw std::runtime_error("no_optimizable_eq_parameters");

        std::vector<double> values;
        values.reserve(bindings.size());
        for (const auto& binding : bindings) {
            values.push_back(binding.Read(snapshot));
        }

        Context context{ &target, &bindings, &snapshot, &cancelRequested, sampleRate };
        nlopt::opt optimizer{ nlopt::LN_COBYLA, static_cast<unsigned>(values.size()) };
        optimizer.set_lower_bounds(std::vector<double>(values.size(), 0.0));
        optimizer.set_upper_bounds(std::vector<double>(values.size(), 1.0));
        optimizer.set_min_objective(&Evaluate, &context);
        optimizer.set_ftol_rel(1e-7);
        optimizer.set_xtol_rel(1e-5);
        optimizer.set_maxeval(2500);

        double loss = std::numeric_limits<double>::infinity();
        try {
            optimizer.optimize(values, loss);
        }
        catch (const nlopt::roundoff_limited&) {
            loss = context.bestLoss;
        }

        if (cancelRequested.load(std::memory_order_acquire)) throw nlopt::forced_stop();
        if (!std::isfinite(context.bestLoss)) throw std::runtime_error("optimizer_failed");
        return { std::move(context.bestSnapshot), context.bestLoss };
    }

private:
    struct Binding final {
        std::size_t filterIndex = 0;
        std::size_t parameterIndex = 0;
        consolidator::models::ParameterRange range;

        double Read(const consolidator::domain::DspSnapshot& snapshot) const {
            const auto* bank = snapshot.eq.SelectedBank();
            return range.Normalize(bank->filters[filterIndex].values[parameterIndex]);
        }

        void Write(consolidator::domain::DspSnapshot& snapshot, double normalized) const {
            auto* bank = snapshot.eq.SelectedBank();
            bank->filters[filterIndex].values[parameterIndex] =
                range.Denormalize(std::clamp(normalized, 0.0, 1.0));
        }
    };

    struct Context final {
        const consolidator::dsp::Curve* target = nullptr;
        const std::vector<Binding>* bindings = nullptr;
        const consolidator::domain::DspSnapshot* baseSnapshot = nullptr;
        const std::atomic<bool>* cancelRequested = nullptr;
        double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz;
        consolidator::domain::DspSnapshot bestSnapshot;
        double bestLoss = std::numeric_limits<double>::infinity();
    };

    static std::vector<Binding> BuildBindings(consolidator::domain::DspSnapshot& snapshot) {
        std::vector<Binding> bindings;
        auto* bank = snapshot.eq.SelectedBank();
        if (!bank) return bindings;

        bank->bypass = false;
        const auto& definitions = consolidator::settings::FilterOptions::EqDefinitions();
        for (std::size_t filterIndex = 0; filterIndex < bank->filters.size(); ++filterIndex) {
            auto& filter = bank->filters[filterIndex];
            const auto definition = definitions.find(filter.filterId);
            if (definition == definitions.end() ||
                filter.values.size() != definition->second.parameters.size()) continue;

            filter.bypass = false;
            for (std::size_t parameterIndex = 0;
                 parameterIndex < definition->second.parameters.size();
                 ++parameterIndex) {
                bindings.push_back({
                    filterIndex,
                    parameterIndex,
                    definition->second.parameters[parameterIndex].range
                });
            }
        }
        return bindings;
    }

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

        auto candidate = *context.baseSnapshot;
        for (std::size_t index = 0; index < values.size(); ++index) {
            (*context.bindings)[index].Write(candidate, values[index]);
        }

        const auto predicted = BuildCurve(candidate, context.target->Settings(), context.sampleRate);
        double loss = 0.0;
        for (std::size_t index = 0; index < predicted.Values().size(); ++index) {
            const auto error = predicted.Values()[index] - context.target->Values()[index];
            loss += PerceptualWeight(context.target->Inputs()[index]) * error * error;
        }
        loss += Regularization(values);
        if (loss < context.bestLoss) {
            context.bestLoss = loss;
            context.bestSnapshot = std::move(candidate);
        }
        return loss;
    }

    static consolidator::dsp::Curve BuildCurve(
        const consolidator::domain::DspSnapshot& snapshot,
        const consolidator::dsp::CurveSettings& settings,
        double sampleRate
    ) {
        consolidator::dsp::Curve curve{ settings };
        const auto* bank = snapshot.eq.SelectedBank();
        if (!bank) return curve;

        const auto& definitions = consolidator::settings::FilterOptions::EqDefinitions();
        for (const auto& filter : bank->filters) {
            if (filter.bypass) continue;
            const auto definition = definitions.find(filter.filterId);
            if (definition == definitions.end()) continue;
            consolidator::dsp::EqFilterFactory factory{
                definition->second, filter.values, sampleRate
            };
            const auto processor = factory.CreateFilter();
            if (!processor) continue;
            for (std::size_t index = 0; index < curve.Values().size(); ++index) {
                curve.AddValue(index, processor->GetMagnitudeDb(curve.Inputs()[index]));
            }
        }
        return curve;
    }

    static double PerceptualWeight(double frequency) noexcept {
        if (frequency < 40.0) return 0.25;
        if (frequency > 16000.0) return 0.35;
        return 1.0;
    }

    static double Regularization(const std::vector<double>& values) noexcept {
        double result = 0.0;
        for (const auto value : values) {
            const auto distance = value - 0.5;
            result += 0.002 * distance * distance;
        }
        return result;
    }
};
