#pragma once

#include "ExperimentManifest.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace emulation_lab
{

void RenderAudioFile(
    const ExperimentManifest& manifest,
    const std::string& side,
    const std::string& conditionId,
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const std::vector<std::pair<std::string, double>>& overrides = {});

void RenderAudioFiles(
    const ExperimentManifest& manifest,
    const std::string& side,
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputDirectory);

} // namespace emulation_lab
