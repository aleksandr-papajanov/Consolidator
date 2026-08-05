#include "Core/Instance/ConsolidatorInstance.h"

#include <cassert>
#include <array>
#include <algorithm>

int main()
{
    consolidator::core::ConsolidatorInstance instance;

    constexpr std::size_t frameCount = 64;
    constexpr std::size_t channels = 2;
    constexpr std::size_t sampleCount = frameCount * channels;

    std::array<double, sampleCount> mainInput{};
    std::array<double, sampleCount> referenceInput{};
    std::array<double, sampleCount> mainOutput{};
    std::array<double, sampleCount> referenceOutput{};

    for (std::size_t i = 0; i < sampleCount; ++i)
    {
        mainInput[i] = static_cast<double>(i);
        referenceInput[i] = static_cast<double>(i + 1000);
    }

    instance.Process(mainInput.data(), referenceInput.data(),
                     mainOutput.data(), referenceOutput.data(),
                     frameCount);

    assert(std::equal(mainInput.begin(), mainInput.end(), mainOutput.begin()));
    assert(std::equal(referenceInput.begin(), referenceInput.end(), referenceOutput.begin()));

    return 0;
}