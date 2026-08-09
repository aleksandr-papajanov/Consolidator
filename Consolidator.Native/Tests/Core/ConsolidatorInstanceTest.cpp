#include "Core/Instance/ConsolidatorInstance.h"
#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/State/StateProtocol.h"

#include <cassert>
#include <array>
#include <algorithm>
#include <chrono>
#include <optional>
#include <thread>

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

    consolidator::core::StateRequestEntries writeEntries;
    consolidator::core::StatePath gainPath;
    gainPath.field = consolidator::core::StateField::DspParameter;
    gainPath.deviceId = consolidator::dsp::DeviceId::MainInputGain;
    gainPath.parameterId = consolidator::dsp::ParameterId::Gain;
    assert(writeEntries.TryAppend({gainPath, 6.0f}));
    instance.EnqueueCommand(consolidator::core::StateCommand{
        consolidator::core::StateOperation::Write,
        {1, instance.GetInstanceId(), writeEntries}});

    std::optional<consolidator::core::StateResponse> response;
    for (std::size_t attempt = 0; attempt < 50 && !response; ++attempt)
    {
        mainInput.fill(1.0);
        instance.Process(mainInput.data(), referenceInput.data(),
                         mainOutput.data(), referenceOutput.data(),
                         frameCount);
        response = consolidator::core::InstanceCoordinator::Get().TryDequeueResponse();
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }

    assert(mainOutput.front() > mainInput.front());
    assert(std::equal(referenceInput.begin(), referenceInput.end(), referenceOutput.begin()));

    assert(response.has_value());
    assert(response->operation == consolidator::core::StateOperation::Write);
    assert(response->entries.size == 1);

    return 0;
}
