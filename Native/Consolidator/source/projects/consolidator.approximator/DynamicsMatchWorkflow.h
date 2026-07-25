#pragma once

#include "ApproximatorFitResult.h"
#include "DeviceOptimizer.h"
#include "FitAudioBuffer.h"

#include <atomic>

class DynamicsMatchWorkflow final {
public:
    ApproximatorFitResult Run(
        consolidator::domain::DspSnapshot snapshot,
        const FitAudioBuffer& audio,
        const std::atomic<bool>& cancelRequested
    ) const {
        const auto result = optimizer.Optimize(
            std::move(snapshot),
            audio,
            cancelRequested,
            consolidator::settings::FitOptions::MaximumEvaluations,
            nullptr,
            DeviceOptimizer::Scope::Dynamics);
        return { result.snapshot, result.loss };
    }

private:
    DeviceOptimizer optimizer;
};
