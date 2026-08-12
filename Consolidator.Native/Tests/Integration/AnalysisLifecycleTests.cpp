#include "Core/Instance/ConsolidatorInstance.h"
#include "Support/ProtocolDriver.h"
#include "Support/StateBuilders.h"
#include "Support/TestFramework.h"

#include <chrono>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <thread>

using namespace consolidator;

namespace
{

analysis::FrequencyResponseSnapshot AwaitEqualizerResponse(
    core::ConsolidatorInstance& instance,
    const std::function<bool(const analysis::FrequencyResponseSnapshot&)>& accept)
{
    for (std::size_t attempt = 0; attempt < 500; ++attempt)
    {
        analysis::FrequencyResponseSnapshot snapshot;
        if (instance.TryReadLatestEqualizerResponse(snapshot) && accept(snapshot))
        {
            return snapshot;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    throw std::runtime_error("equalizer response timeout");
}

} // namespace

TEST_CASE("Analysis slot unregisters with the instance lifecycle")
{
    {
        core::ConsolidatorInstance instance;
        instance.Initialize();
        EXPECT_TRUE(instance.GetStateStore().GetInstanceId() ==
                    instance.GetInstanceId());
    }

    core::ConsolidatorInstance replacement;
    replacement.Initialize();
    EXPECT_TRUE(replacement.GetStateStore().GetInstanceId() ==
                replacement.GetInstanceId());
}

TEST_CASE("Instance publishes live spectrum after a complete audio window")
{
    test::ProtocolDriver driver{1};
    auto& instance = driver.At(0);
    instance.Prepare(48000.0);
    driver.MainInput().fill(1.0);

    for (std::size_t block = 0;
         block < analysis::kFftSize / 16;
         ++block)
    {
        driver.ProcessAll();
    }

    analysis::SpectrumSnapshot snapshot;
    bool received = false;
    for (std::size_t attempt = 0; attempt < 500 && !received; ++attempt)
    {
        received = instance.TryReadLatestSpectrum(snapshot);
        if (!received)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{2});
        }
    }

    EXPECT_TRUE(received);
    EXPECT_NEAR(snapshot.sampleRate, 48000.0, 0.0);
    EXPECT_TRUE(snapshot.revision > 0);
}

TEST_CASE("Chain solo republishes a flat equalizer response")
{
    test::ProtocolDriver driver{1};
    auto& instance = driver.At(0);
    instance.Prepare(48000.0);
    const auto instanceId = instance.GetInstanceId();

    (void)driver.Write(0, 3000, test::Entries({test::Write(
        test::FilterPath(
            instanceId,
            dsp::BankId::Bank0,
            0,
            dsp::ParameterId::Gain),
        6.0F)}));
    const auto boosted = AwaitEqualizerResponse(
        instance,
        [](const auto& snapshot)
        {
            return std::abs(snapshot.magnitudeDb[0] - 6.0F) < 0.05F;
        });

    (void)driver.Write(0, 3001, test::Entries({test::Write(
        test::DevicePath(
            instanceId,
            dsp::DeviceId::Compressor,
            core::StateMarkerId::Solo),
        true)}));
    const auto bypassed = AwaitEqualizerResponse(
        instance,
        [&boosted](const auto& snapshot)
        {
            return snapshot.revision > boosted.revision &&
                std::abs(snapshot.magnitudeDb[0]) < 0.05F;
        });

    EXPECT_TRUE(bypassed.revision > boosted.revision);
}

TEST_MAIN()
