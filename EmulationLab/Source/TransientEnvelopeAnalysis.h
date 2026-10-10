#pragma once
#include <filesystem>
namespace emulation_lab
{
void AnalyzeTransientEnvelopes(const std::filesystem::path& dryPath,
    const std::filesystem::path& referencePath,
    const std::filesystem::path& candidatePath,
    const std::filesystem::path& outputPath);
}
