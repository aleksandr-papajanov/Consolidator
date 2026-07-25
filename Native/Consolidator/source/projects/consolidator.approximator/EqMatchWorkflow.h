#pragma once

#include "ApproximatorFitResult.h"
#include "EqOptimizer.h"
#include "DSP/Eq/EqRuntime.h"
#include "Settings/AudioOptions.h"

#include <stdexcept>

class EqMatchWorkflow final {
public:
    ApproximatorFitResult Run(
        const consolidator::dsp::Curve& residual,
        consolidator::domain::DspSnapshot snapshot,
        const EqOptimizer::Definitions& definitions
    ) const {
        consolidator::dsp::EqRuntime runtime;
        runtime.SetSnapshot(snapshot.eq);
        const auto selectedBankResponse = runtime.BuildBankCurve(
            snapshot.eq.selectedBankId,
            consolidator::settings::AudioOptions::DefaultSampleRateHz);
        const auto target = selectedBankResponse + residual;
        const auto fit = optimizer.Fit(target, definitions);
        ApplyFilterValues(snapshot, fit, definitions);
        return { std::move(snapshot), fit.loss };
    }

private:
    static void ApplyFilterValues(
        consolidator::domain::DspSnapshot& snapshot,
        const EqOptimizer::FitResult& fit,
        const EqOptimizer::Definitions& definitions
    ) {
        auto* bank = snapshot.eq.SelectedBank();
        if (!bank) throw std::runtime_error("invalid_fit_result");

        std::size_t offset = 0;
        for (auto& filter : bank->filters) {
            const auto definition = definitions.find(filter.filterId);
            if (definition == definitions.end()) {
                throw std::runtime_error("fit_definition_missing");
            }
            filter.bypass = false;
            filter.values.clear();
            filter.values.reserve(definition->second.parameters.size());
            for (const auto& parameter : definition->second.parameters) {
                if (offset >= fit.solverValues.size()) {
                    throw std::runtime_error("fit_result_size_mismatch");
                }
                filter.values.push_back(
                    parameter.range.Denormalize(fit.solverValues[offset++]));
            }
        }
        if (offset != fit.solverValues.size()) {
            throw std::runtime_error("fit_result_size_mismatch");
        }
    }

    EqOptimizer optimizer;
};
