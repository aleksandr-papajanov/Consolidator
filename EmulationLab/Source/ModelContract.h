#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace emulation_lab
{

struct ModelParameter
{
    std::string identifier;
    double lowerBound{};
    double upperBound{};
};

struct ModelEvaluation
{
    double objective{ std::numeric_limits<double>::infinity() };
    std::vector<double> diagnostics;
};

class IEmulationProblem
{
public:
    virtual ~IEmulationProblem() = default;

    [[nodiscard]] virtual std::span<const ModelParameter> Parameters() const noexcept = 0;
    [[nodiscard]] virtual ModelEvaluation Evaluate(
        std::span<const double> parameterValues) const = 0;
};

struct ModelCandidate
{
    std::vector<double> parameterValues;
    ModelEvaluation evaluation;
};

} // namespace emulation_lab