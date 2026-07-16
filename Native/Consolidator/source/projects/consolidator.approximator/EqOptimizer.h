#pragma once

#include "EqFrequencyGrid.h"
#include "EqModel.h"
#include "FilterRegistry.h"
#include "TargetCurve.h"

#include <nlopt.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

class EqOptimizer {
public:
    struct FitResult {
        std::vector<double> normalized_values;
        double loss = 0.0;
    };

    FitResult fit(
        const TargetCurve& curve,
        const FilterRegistry& registry,
        double sample_rate = EqCurveGrid::default_sample_rate
    ) const {
        if (curve.frequencies.size() != curve.values.size()) {
            throw std::runtime_error("freqs and targetDb size mismatch");
        }

        if (curve.frequencies.empty()) {
            throw std::runtime_error("empty curve");
        }

        std::vector<double> initial;
        for (const auto& contract : registry.all()) {
            if (!contract) {
                continue;
            }

            const auto defaults = default_normalized_values(*contract);
            initial.insert(initial.end(), defaults.begin(), defaults.end());
        }

        if (initial.empty()) {
            throw std::runtime_error("no_optimizable_parameters");
        }

        FitContext context{ &curve, &registry, sample_rate };
        std::vector<double> lower(initial.size(), 0.0);
        std::vector<double> upper(initial.size(), 1.0);

        nlopt::opt optimizer(
            nlopt::LN_COBYLA,
            static_cast<unsigned int>(initial.size()));
        optimizer.set_lower_bounds(lower);
        optimizer.set_upper_bounds(upper);
        optimizer.set_min_objective(&evaluate, &context);
        optimizer.set_ftol_rel(1e-7);
        optimizer.set_xtol_rel(1e-5);
        optimizer.set_maxeval(2500);

        double final_loss = 0.0;
        try {
            optimizer.optimize(initial, final_loss);
        }
        catch (const nlopt::roundoff_limited&) {
            std::vector<double> gradient;
            final_loss = evaluate(initial, gradient, &context);
        }

        if (!std::isfinite(final_loss)) {
            throw std::runtime_error("optimizer_failed");
        }

        return { std::move(initial), final_loss };
    }

private:
    struct FitContext {
        const TargetCurve* curve;
        const FilterRegistry* registry;
        double sample_rate;
    };

    static double evaluate(
        const std::vector<double>& values,
        std::vector<double>& gradient,
        void* user_data
    ) {
        (void)gradient;
        const auto& context = *static_cast<const FitContext*>(user_data);
        const auto predicted = EqModel::buildCurve(
            context.curve->frequencies,
            *context.registry,
            values,
            context.sample_rate);

        double total = 0.0;
        for (std::size_t i = 0; i < predicted.size(); ++i) {
            const double error = predicted[i] - context.curve->values[i];
            total += perceptual_weight(context.curve->frequencies[i]) * error * error;
        }

        total += regularization(values, *context.registry);
        return total;
    }

    static double perceptual_weight(double frequency) {
        if (frequency < 40.0) {
            return 0.25;
        }

        if (frequency > 16000.0) {
            return 0.35;
        }

        return 1.0;
    }

    static double regularization(
        const std::vector<double>& values,
        const FilterRegistry& registry
    ) {
        std::vector<double> defaults;
        for (const auto& contract : registry.all()) {
            if (!contract) {
                continue;
            }

            const auto contract_defaults = default_normalized_values(*contract);
            defaults.insert(defaults.end(), contract_defaults.begin(), contract_defaults.end());
        }

        double result = 0.0;
        for (std::size_t i = 0; i < values.size() && i < defaults.size(); ++i) {
            const double distance = values[i] - defaults[i];
            result += 0.002 * distance * distance;
        }

        return result;
    }
};
