#pragma once

#include "ModelContract.h"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <vector>

namespace emulation_lab
{

struct OptimizationOptions
{
    int globalEvaluationLimit{ 1000 };
    int localEvaluationLimit{ 250 };
    unsigned int population{};
    std::size_t beamWidth{ 8 };
    unsigned long randomSeed{ 5489 };
};

struct OptimizationResult
{
    ModelCandidate best;
    std::vector<ModelCandidate> beam;
    std::size_t evaluationCount{};
};

using ProgressCallback = std::function<void(
    std::size_t evaluationCount,
    const ModelCandidate& best)>;

[[nodiscard]] OptimizationResult Optimize(
    const IEmulationProblem& problem,
    const OptimizationOptions& options,
    const std::vector<std::vector<double>>& initialCandidates,
    ProgressCallback progressCallback = {});

[[nodiscard]] bool RunOptimizerSelfTest();

void SaveOptimizationResult(
    const OptimizationResult& result,
    std::span<const ModelParameter> parameters,
    const std::filesystem::path& path);

} // namespace emulation_lab