#pragma once

#include "Capture.h"
#include "ExperimentManifest.h"
#include "ModelContract.h"

#include <memory>
#include <string>
#include <vector>

namespace emulation_lab
{

class PluginParameterProblem final : public IEmulationProblem
{
public:
    PluginParameterProblem(
        ExperimentManifest manifest,
        MeasurementDataset reference,
        std::vector<std::string> parameterPatterns,
        std::vector<std::string> probes,
        std::vector<double> baseValues,
        std::size_t maximumObservations = 0);

    [[nodiscard]] std::span<const ModelParameter> Parameters() const noexcept override;
    [[nodiscard]] ModelEvaluation Evaluate(
        std::span<const double> parameterValues) const override;

    [[nodiscard]] const std::vector<std::size_t>& ParameterIndices() const noexcept;

private:
    ExperimentManifest manifest_;
    MeasurementDataset reference_;
    std::vector<std::string> probes_;
    std::vector<double> baseValues_;
    std::vector<std::size_t> parameterIndices_;
    std::vector<ModelParameter> parameters_;
    mutable std::unique_ptr<PluginCaptureSession> captureSession_;
};

[[nodiscard]] bool PatternMatches(const std::string& pattern, const std::string& value);

} // namespace emulation_lab
