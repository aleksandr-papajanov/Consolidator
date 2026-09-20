#include "EmulationOptimizer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include <nlohmann/json.hpp>
#include <nlopt.hpp>

namespace emulation_lab
{

namespace
{

struct OptimizationContext
{
    const IEmulationProblem* problem{};
    const OptimizationOptions* options{};
    std::vector<ModelCandidate> beam;
    std::size_t evaluationCount{};
    ProgressCallback progressCallback;
};

void SortBeam(std::vector<ModelCandidate>& beam)
{
    std::sort(
        beam.begin(),
        beam.end(),
        [](const auto& left, const auto& right)
        {
            return left.evaluation.objective < right.evaluation.objective;
        });
}

void Validate(
    const IEmulationProblem& problem,
    const std::vector<std::vector<double>>& initialCandidates)
{
    const auto parameters = problem.Parameters();
    if (parameters.empty() || initialCandidates.empty())
    {
        throw std::invalid_argument("An emulation problem needs parameters and an initial candidate.");
    }

    std::unordered_set<std::string> identifiers;
    for (const auto& parameter : parameters)
    {
        if (parameter.identifier.empty() ||
            !std::isfinite(parameter.lowerBound) ||
            !std::isfinite(parameter.upperBound) ||
            parameter.lowerBound >= parameter.upperBound ||
            !identifiers.insert(parameter.identifier).second)
        {
            throw std::invalid_argument("An emulation parameter descriptor is invalid.");
        }
    }
    for (const auto& candidate : initialCandidates)
    {
        if (candidate.size() != parameters.size())
        {
            throw std::invalid_argument("An initial candidate has the wrong parameter count.");
        }
        for (std::size_t index = 0; index < candidate.size(); ++index)
        {
            if (!std::isfinite(candidate[index]) ||
                candidate[index] < parameters[index].lowerBound ||
                candidate[index] > parameters[index].upperBound)
            {
                throw std::invalid_argument("An initial candidate is outside the parameter bounds.");
            }
        }
    }
}

void InsertCandidate(
    std::vector<ModelCandidate>& beam,
    ModelCandidate candidate,
    std::span<const ModelParameter> parameters,
    std::size_t beamWidth)
{
    for (auto& existing : beam)
    {
        auto distanceSquared = 0.0;
        for (std::size_t index = 0; index < parameters.size(); ++index)
        {
            const auto range = parameters[index].upperBound - parameters[index].lowerBound;
            const auto difference =
                (candidate.parameterValues[index] - existing.parameterValues[index]) / range;
            distanceSquared += difference * difference;
        }
        constexpr double kMinimumNormalizedDistanceSquared = 1.0e-4;
        if (distanceSquared < kMinimumNormalizedDistanceSquared)
        {
            if (candidate.evaluation.objective < existing.evaluation.objective)
            {
                existing = std::move(candidate);
                SortBeam(beam);
            }
            return;
        }
    }

    beam.push_back(std::move(candidate));
    SortBeam(beam);
    if (beam.size() > std::max<std::size_t>(beamWidth, 1))
    {
        beam.resize(std::max<std::size_t>(beamWidth, 1));
    }
}

double EvaluateCandidate(
    const std::vector<double>& values,
    std::vector<double>&,
    void* opaqueContext)
{
    auto& context = *static_cast<OptimizationContext*>(opaqueContext);
    auto evaluation = context.problem->Evaluate(values);
    if (!std::isfinite(evaluation.objective))
    {
        throw std::runtime_error("The emulation problem returned a non-finite objective.");
    }

    const auto objective = evaluation.objective;
    InsertCandidate(
        context.beam,
        ModelCandidate{ values, std::move(evaluation) },
        context.problem->Parameters(),
        context.options->beamWidth);
    ++context.evaluationCount;
    if (context.progressCallback)
    {
        context.progressCallback(context.evaluationCount, context.beam.front());
    }
    return objective;
}

void SetBounds(
    nlopt::opt& optimizer,
    std::span<const ModelParameter> parameters)
{
    std::vector<double> lowerBounds;
    std::vector<double> upperBounds;
    lowerBounds.reserve(parameters.size());
    upperBounds.reserve(parameters.size());
    for (const auto& parameter : parameters)
    {
        lowerBounds.push_back(parameter.lowerBound);
        upperBounds.push_back(parameter.upperBound);
    }
    optimizer.set_lower_bounds(lowerBounds);
    optimizer.set_upper_bounds(upperBounds);
}

class QuadraticProblem final : public IEmulationProblem
{
public:
    [[nodiscard]] std::span<const ModelParameter> Parameters() const noexcept override
    {
        return parameters_;
    }

    [[nodiscard]] ModelEvaluation Evaluate(
        std::span<const double> values) const override
    {
        const auto firstError = values[0] - 0.25;
        const auto secondError = values[1] + 0.5;
        return {
            firstError * firstError + secondError * secondError,
            { firstError, secondError }
        };
    }

private:
    const std::array<ModelParameter, 2> parameters_{ {
        { "first", -1.0, 1.0 },
        { "second", -1.0, 1.0 }
    } };
};

} // namespace

OptimizationResult Optimize(
    const IEmulationProblem& problem,
    const OptimizationOptions& options,
    const std::vector<std::vector<double>>& initialCandidates,
    ProgressCallback progressCallback)
{
    Validate(problem, initialCandidates);
    OptimizationContext context{ &problem, &options, {}, 0, std::move(progressCallback) };
    std::vector<double> gradient;
    for (const auto& candidate : initialCandidates)
    {
        EvaluateCandidate(candidate, gradient, &context);
    }

    auto values = context.beam.front().parameterValues;
    auto minimum = context.beam.front().evaluation.objective;
    const auto parameters = problem.Parameters();
    nlopt::srand(options.randomSeed);
    const auto parameterCount = static_cast<unsigned int>(parameters.size());

    nlopt::opt globalOptimizer(nlopt::GN_CRS2_LM, parameterCount);
    SetBounds(globalOptimizer, parameters);
    globalOptimizer.set_min_objective(EvaluateCandidate, &context);
    globalOptimizer.set_maxeval(std::max(options.globalEvaluationLimit, 1));
    if (options.population > 0)
    {
        globalOptimizer.set_population(options.population);
    }
    try
    {
        globalOptimizer.optimize(values, minimum);
    }
    catch (const nlopt::roundoff_limited&)
    {
    }

    const auto globalBeam = context.beam;
    for (const auto& candidate : globalBeam)
    {
        values = candidate.parameterValues;
        minimum = candidate.evaluation.objective;
        nlopt::opt localOptimizer(nlopt::LN_BOBYQA, parameterCount);
        SetBounds(localOptimizer, parameters);
        localOptimizer.set_min_objective(EvaluateCandidate, &context);
        localOptimizer.set_maxeval(std::max(options.localEvaluationLimit, 1));
        localOptimizer.set_xtol_rel(1.0e-5);
        try
        {
            localOptimizer.optimize(values, minimum);
        }
        catch (const nlopt::roundoff_limited&)
        {
        }
    }

    SortBeam(context.beam);
    return { context.beam.front(), std::move(context.beam), context.evaluationCount };
}

bool RunOptimizerSelfTest()
{
    QuadraticProblem problem;
    OptimizationOptions options;
    options.globalEvaluationLimit = 8;
    options.localEvaluationLimit = 8;
    options.beamWidth = 2;
    const auto result = Optimize(problem, options, { { 0.9, 0.9 } });
    const auto checkpoint = std::filesystem::temp_directory_path() /
        "emulation-lab-optimizer-self-test.json";
    SaveOptimizationResult(result, problem.Parameters(), checkpoint);
    const auto checkpointWritten = std::filesystem::exists(checkpoint);
    std::filesystem::remove(checkpoint);
    return checkpointWritten &&
        !result.beam.empty() &&
        result.beam.size() <= options.beamWidth &&
        result.best.evaluation.objective < 0.1 &&
        result.best.parameterValues.size() == problem.Parameters().size();
}

void SaveOptimizationResult(
    const OptimizationResult& result,
    std::span<const ModelParameter> parameters,
    const std::filesystem::path& path)
{
    if (parameters.size() != result.best.parameterValues.size())
    {
        throw std::invalid_argument("Optimization result and parameter schema differ.");
    }
    const auto parent = path.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }
    nlohmann::json document{
        { "schemaVersion", 1 },
        { "evaluationCount", result.evaluationCount },
        { "best", {
            { "objective", result.best.evaluation.objective },
            { "parameters", nlohmann::json::array() },
            { "diagnostics", result.best.evaluation.diagnostics }
        } },
        { "beam", nlohmann::json::array() }
    };
    for (std::size_t index = 0; index < parameters.size(); ++index)
    {
        document["best"]["parameters"].push_back({
            { "id", parameters[index].identifier },
            { "value", result.best.parameterValues[index] }
        });
    }
    for (const auto& candidate : result.beam)
    {
        document["beam"].push_back({
            { "objective", candidate.evaluation.objective },
            { "parameters", candidate.parameterValues },
            { "diagnostics", candidate.evaluation.diagnostics }
        });
    }
    std::ofstream output(path);
    if (!output)
    {
        throw std::runtime_error("Could not write optimization result: " + path.string());
    }
    output << document.dump(2) << '\n';
}

} // namespace emulation_lab
