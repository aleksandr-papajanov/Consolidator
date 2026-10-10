#pragma once

#include <filesystem>
#include <string>

#include "ExperimentManifest.h"

namespace emulation_lab
{

void RunDiagnostics(
    const ExperimentManifest& manifest,
    const std::string& side,
    const std::string& conditionId,
    const std::filesystem::path& outputPath,
    const std::filesystem::path& fitPath);


} // namespace emulation_lab
