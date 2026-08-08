#include "Core/Instance/ConsolidatorInstance.h"
#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/Parameters/RoutedParameterChange.h"

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

    instance.EnqueueCommand(consolidator::core::ChangeDspParameterCommand{
        consolidator::dsp::ParameterRoute{
            consolidator::dsp::DeviceId::MainInputGain,
            consolidator::dsp::ParameterId::Gain},
        consolidator::dsp::ParameterValue{6.0f}});

    mainInput.fill(1.0);
    instance.Process(mainInput.data(), referenceInput.data(),
                     mainOutput.data(), referenceOutput.data(),
                     frameCount);

    assert(mainOutput.front() > mainInput.front());
    assert(std::equal(referenceInput.begin(), referenceInput.end(), referenceOutput.begin()));

    return 0;
}
