#include "Core/Instance/ConsolidatorInstance.h"

#include <algorithm>

namespace consolidator::core
{

ConsolidatorInstance::ConsolidatorInstance() = default;

ConsolidatorInstance::~ConsolidatorInstance() = default;

void ConsolidatorInstance::Process(const double* mainInput,
                                   const double* referenceInput,
                                   double* mainOutput,
                                   double* referenceOutput,
                                   std::size_t frameCount)
{
    // Passthrough: copy input directly to output.
    const std::size_t sampleCount = frameCount * kChannelCount;

    std::copy_n(mainInput, sampleCount, mainOutput);
    std::copy_n(referenceInput, sampleCount, referenceOutput);
}

void ConsolidatorInstance::ApplyParameterChange(const dsp::ParameterChange&)
{
    // Placeholder. Parameter application will be added with the DSP layer.
}

} // namespace consolidator::core
