#pragma once

#include "Dataset.h"
#include "ExperimentManifest.h"
#include "RunMetadata.h"

#include <memory>
#include <string>
#include <vector>

namespace emulation_lab
{

struct CaptureResult
{
    MeasurementDataset dataset;
    RunMetadata metadata;
};

struct PluginParameterValue
{
    std::string name;
    double normalizedValue{};
};

class PluginCaptureSession final
{
public:
    PluginCaptureSession(
        ExperimentManifest manifest,
        std::string side,
        std::vector<std::string> selectedProbes = {},
        std::vector<std::string> selectedObservationIds = {},
        bool includeMetadata = true);
    ~PluginCaptureSession();

    PluginCaptureSession(const PluginCaptureSession&) = delete;
    PluginCaptureSession& operator=(const PluginCaptureSession&) = delete;

    [[nodiscard]] CaptureResult Capture(
        const std::vector<PluginParameterValue>& parameterValues = {});

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] CaptureResult CapturePlugin(
    const ExperimentManifest& manifest,
    const std::string& side,
    const std::vector<std::string>& selectedProbes = {},
    const std::vector<PluginParameterValue>& parameterValues = {},
    bool includeMetadata = true);

[[nodiscard]] MeasurementDataset FilterDataset(
    const MeasurementDataset& source,
    const std::vector<std::string>& probes);

} // namespace emulation_lab
