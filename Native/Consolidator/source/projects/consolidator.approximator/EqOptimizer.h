#pragma once

#include "DSP/Curve/Curve.h"
#include "DSP/Eq/EqFilterFactory.h"
#include "Models/FilterDefinition.h"
#include "Settings/AudioOptions.h"

#include <nlopt.hpp>

#include <cmath>
#include <map>
#include <stdexcept>
#include <vector>

class EqOptimizer final {
public:
    using Definitions = std::map<long, consolidator::models::FilterDefinition>;

    struct FitResult final {
        std::vector<double> solverValues;
        double loss = 0.0;
    };

    FitResult Fit(
        const consolidator::dsp::Curve& target,
        const Definitions& definitions,
        double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz
    ) const {
        std::vector<double> values;
        for (const auto& [filterId, definition] : definitions) {
            (void)filterId;
            for (const auto& parameter : definition.parameters) {
                values.push_back(parameter.range.Normalize(parameter.defaultValue));
            }
        }
        if (values.empty()) throw std::runtime_error("no_optimizable_parameters");

        FitContext context{ &target, &definitions, sampleRate };
        nlopt::opt optimizer{ nlopt::LN_COBYLA, static_cast<unsigned>(values.size()) };
        optimizer.set_lower_bounds(std::vector<double>(values.size(), 0.0));
        optimizer.set_upper_bounds(std::vector<double>(values.size(), 1.0));
        optimizer.set_min_objective(&Evaluate, &context);
        optimizer.set_ftol_rel(1e-7);
        optimizer.set_xtol_rel(1e-5);
        optimizer.set_maxeval(2500);

        double loss = 0.0;
        try {
            optimizer.optimize(values, loss);
        }
        catch (const nlopt::roundoff_limited&) {
            std::vector<double> gradient;
            loss = Evaluate(values, gradient, &context);
        }
        if (!std::isfinite(loss)) throw std::runtime_error("optimizer_failed");
        return { std::move(values), loss };
    }

private:
    struct FitContext final {
        const consolidator::dsp::Curve* target;
        const Definitions* definitions;
        double sampleRate;
    };

    static double Evaluate(
        const std::vector<double>& values,
        std::vector<double>& gradient,
        void* contextPointer
    ) {
        (void)gradient;
        const auto& context = *static_cast<const FitContext*>(contextPointer);
        const auto predicted = BuildCurve(
            *context.target, *context.definitions, values, context.sampleRate);
        double loss = 0.0;
        for (std::size_t index = 0; index < predicted.Values().size(); ++index) {
            const auto error = predicted.Values()[index] - context.target->Values()[index];
            loss += PerceptualWeight(context.target->Inputs()[index]) * error * error;
        }
        return loss + Regularization(values, *context.definitions);
    }

    static consolidator::dsp::Curve BuildCurve(
        const consolidator::dsp::Curve& target,
        const Definitions& definitions,
        const std::vector<double>& solverValues,
        double sampleRate
    ) {
        consolidator::dsp::Curve result{ target.Settings() };
        std::size_t offset = 0;
        for (const auto& [filterId, definition] : definitions) {
            (void)filterId;
            std::vector<double> absoluteValues;
            absoluteValues.reserve(definition.parameters.size());
            for (const auto& parameter : definition.parameters) {
                if (offset >= solverValues.size()) return result;
                absoluteValues.push_back(parameter.range.Denormalize(solverValues[offset++]));
            }
            consolidator::dsp::EqFilterFactory factory{
                definition, absoluteValues, sampleRate
            };
            const auto filter = factory.CreateFilter();
            if (!filter) continue;
            for (std::size_t index = 0; index < result.Values().size(); ++index) {
                result.AddValue(index, filter->GetMagnitudeDb(target.Inputs()[index]));
            }
        }
        return result;
    }

    static double PerceptualWeight(double frequency) noexcept {
        if (frequency < 40.0) return 0.25;
        if (frequency > 16000.0) return 0.35;
        return 1.0;
    }

    static double Regularization(
        const std::vector<double>& values,
        const Definitions& definitions
    ) noexcept {
        double result = 0.0;
        std::size_t offset = 0;
        for (const auto& [filterId, definition] : definitions) {
            (void)filterId;
            for (const auto& parameter : definition.parameters) {
                if (offset >= values.size()) return result;
                const auto defaultValue = parameter.range.Normalize(parameter.defaultValue);
                const auto distance = values[offset++] - defaultValue;
                result += 0.002 * distance * distance;
            }
        }
        return result;
    }
};
