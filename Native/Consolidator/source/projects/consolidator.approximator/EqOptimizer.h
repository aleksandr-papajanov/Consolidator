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

class EqOptimizer {
public:
    using Definitions = std::map<long, consolidator::models::FilterDefinition>;

    struct FitResult {
        std::vector<double> solverValues;
        double loss = 0.0;
    };

    FitResult Fit(
        const consolidator::dsp::Curve& curve,
        const Definitions& definitions,
        double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz
    ) const {
        std::vector<double> initial;
        for (const auto& [filterId, definition] : definitions) {
            (void)filterId;
            for (const auto& parameter : definition.parameters) {
                initial.push_back(parameter.range.Normalize(parameter.defaultValue));
            }
        }
        if (initial.empty()) throw std::runtime_error("no_optimizable_parameters");

        FitContext context{ &curve, &definitions, sampleRate };
        nlopt::opt optimizer{ nlopt::LN_COBYLA, static_cast<unsigned int>(initial.size()) };
        optimizer.set_lower_bounds(std::vector<double>(initial.size(), 0.0));
        optimizer.set_upper_bounds(std::vector<double>(initial.size(), 1.0));
        optimizer.set_min_objective(&Evaluate, &context);
        optimizer.set_ftol_rel(1e-7);
        optimizer.set_xtol_rel(1e-5);
        optimizer.set_maxeval(2500);

        double loss = 0.0;
        try {
            optimizer.optimize(initial, loss);
        }
        catch (const nlopt::roundoff_limited&) {
            std::vector<double> gradient;
            loss = Evaluate(initial, gradient, &context);
        }
        if (!std::isfinite(loss)) throw std::runtime_error("optimizer_failed");
        return { std::move(initial), loss };
    }

private:
    struct FitContext {
        const consolidator::dsp::Curve* curve;
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
        const auto predicted = BuildCurve(*context.curve, *context.definitions, values, context.sampleRate);
        double loss = 0.0;
        for (std::size_t index = 0; index < predicted.Values().size(); ++index) {
            const double error = predicted.Values()[index] - context.curve->Values()[index];
            loss += PerceptualWeight(context.curve->Inputs()[index]) * error * error;
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
            consolidator::dsp::EqFilterFactory factory{ definition, absoluteValues, sampleRate };
            const auto filter = factory.CreateFilter();
            if (!filter) continue;
            for (std::size_t index = 0; index < result.Values().size(); ++index) {
                result.AddValue(index, filter->GetMagnitudeDb(target.Inputs()[index]));
            }
        }
        return result;
    }

    static double PerceptualWeight(double frequency) {
        if (frequency < 40.0) return 0.25;
        if (frequency > 16000.0) return 0.35;
        return 1.0;
    }

    static double Regularization(
        const std::vector<double>& values,
        const Definitions& definitions
    ) {
        double result = 0.0;
        std::size_t offset = 0;
        for (const auto& [filterId, definition] : definitions) {
            (void)filterId;
            for (const auto& parameter : definition.parameters) {
                if (offset >= values.size()) return result;
                const double distance = values[offset++] -
                    parameter.range.Normalize(parameter.defaultValue);
                result += 0.002 * distance * distance;
            }
        }
        return result;
    }
};
