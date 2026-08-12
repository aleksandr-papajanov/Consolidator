#include "Analysis/AnalysisService.h"
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

analysis::EqualizerCurveSnapshot AwaitEqualizerResponse(
    const std::function<bool(const analysis::EqualizerCurveSnapshot&)>& accept)
{
    analysis::AnalysisView view;
    for (std::size_t attempt = 0; attempt < 500; ++attempt)
    {
        analysis::EqualizerCurveSnapshot snapshot;
        if (analysis::AnalysisService::Get().TryReadLatestCurve(
                snapshot, 0, view) &&
            accept(snapshot))
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
    EXPECT_TRUE(
        instance.GetStateStore().GetInstanceId() == instance.GetInstanceId());
    }

    core::ConsolidatorInstance replacement;
    replacement.Initialize();
    EXPECT_TRUE(
        replacement.GetStateStore().GetInstanceId() ==
        replacement.GetInstanceId());
}

TEST_CASE("Instance publishes live spectrum after a complete audio window")
{
    test::ProtocolDriver driver{1};
    auto& instance = driver.At(0);
    instance.Prepare(48000.0);
    analysis::AnalysisService::Get().SetView(
        {instance.GetInstanceId(), dsp::BankId::Bank0});
    driver.MainInput().fill(1.0);

    for (std::size_t block = 0;
         block < analysis::kFftSize / 16;
         ++block)
    {
        driver.ProcessAll();
    }

    analysis::SpectrumSnapshot snapshot;
    analysis::AnalysisView resultView;
    bool received = false;
    for (std::size_t attempt = 0; attempt < 500 && !received; ++attempt)
    {
        received = analysis::AnalysisService::Get().TryReadLatestSpectrum(
            snapshot, 0, resultView);
        if (!received)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{2});
        }
    }

    EXPECT_TRUE(received);
    EXPECT_NEAR(snapshot.sampleRate, 48000.0, 0.0);
    EXPECT_TRUE(snapshot.revision > 0);
}

TEST_CASE("Instance publishes main-reference difference spectrum")
{
    test::ProtocolDriver driver{1};
    auto& instance = driver.At(0);
    instance.Prepare(48000.0);
    analysis::AnalysisService::Get().SetView(
        {instance.GetInstanceId(), dsp::BankId::Bank0});
    driver.MainInput().fill(1.0);
    driver.ReferenceInput().fill(0.5);

    for (std::size_t block = 0;
         block < analysis::kFftSize / 16;
         ++block)
    {
        driver.ProcessAll();
    }

    analysis::SpectrumSnapshot snapshot;
    analysis::AnalysisView resultView;
    bool received = false;
    for (std::size_t attempt = 0; attempt < 500 && !received; ++attempt)
    {
        received = analysis::AnalysisService::Get()
            .TryReadLatestDifferenceSpectrum(snapshot, 0, resultView);
        if (!received)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{2});
        }
    }

    EXPECT_TRUE(received);
    EXPECT_NEAR(snapshot.magnitudeDb[0], 6.02, 0.1);

    analysis::SpectrumSnapshot mainSnapshot;
    analysis::SpectrumSnapshot referenceSnapshot;
    EXPECT_TRUE(
        analysis::AnalysisService::Get().TryReadLatestSpectrum(
            mainSnapshot, 0, resultView));
    EXPECT_TRUE(analysis::AnalysisService::Get().TryReadLatestReferenceSpectrum(
        referenceSnapshot, 0, resultView));
    EXPECT_TRUE(mainSnapshot.sourceRevision > 0);
    EXPECT_TRUE(referenceSnapshot.sourceRevision > 0);
    analysis::SpectrumSnapshot repeatedMainSnapshot;
    EXPECT_TRUE(
        analysis::AnalysisService::Get().TryReadLatestSpectrum(
            repeatedMainSnapshot, 0, resultView));
    EXPECT_EQ(repeatedMainSnapshot.revision, mainSnapshot.revision);
    EXPECT_FALSE(
        analysis::AnalysisService::Get().TryReadLatestSpectrum(
            repeatedMainSnapshot, mainSnapshot.revision, resultView));
}

TEST_CASE("Chain solo republishes a flat equalizer response")
{
    test::ProtocolDriver driver{1};
    auto& instance = driver.At(0);
    instance.Prepare(48000.0);
    analysis::AnalysisService::Get().SetView({instance.GetInstanceId(), dsp::BankId::Bank0});
    const auto instanceId = instance.GetInstanceId();

    (void)driver.Write(0, 3000, test::Entries({test::Write(test::FilterPath(instanceId, dsp::BankId::Bank0, 0, dsp::ParameterId::Gain), 6.0F)}));
    const auto boosted = AwaitEqualizerResponse(
        [](const auto& snapshot)
        {
            return std::abs(snapshot.combined.magnitudeDb[0] - 6.0F) < 0.05F;
        });

    (void)driver.Write(0, 3001, test::Entries({test::Write(test::DevicePath(instanceId, dsp::DeviceId::Compressor, core::StateMarkerId::Solo), true)}));
    const auto bypassed = AwaitEqualizerResponse(
        [&boosted](const auto& snapshot)
        {
            return snapshot.revision > boosted.revision && std::abs(snapshot.combined.magnitudeDb[0]) < 0.05F;
        });

    EXPECT_TRUE(bypassed.revision > boosted.revision);
}

TEST_CASE("Prepare republishes curve input after a sample-rate change")
{
    test::ProtocolDriver driver{1};
    auto& instance = driver.At(0);
    instance.Prepare(48000.0);
    analysis::AnalysisService::Get().SetView(
        {instance.GetInstanceId(), dsp::BankId::Bank0});

    const auto initial = AwaitEqualizerResponse(
        [](const auto& snapshot)
        {
            return snapshot.combined.sourceRevision > 0;
        });

    instance.Prepare(96000.0);
    const auto updated = AwaitEqualizerResponse(
        [&initial](const auto& snapshot)
        {
            return snapshot.combined.sourceRevision >
                initial.combined.sourceRevision;
        });

    EXPECT_TRUE(updated.revision > initial.revision);
}

TEST_CASE("Changing the analysis bank publishes all curve variants")
{
    test::ProtocolDriver driver{1};
    auto& instance = driver.At(0);
    instance.Prepare(48000.0);
    const auto instanceId = instance.GetInstanceId();
    analysis::AnalysisService::Get().SetView({instanceId, dsp::BankId::Bank0});

    (void)driver.Write(0, 4000, test::Entries({test::Write(
        test::FilterPath(
            instanceId,
            dsp::BankId::Bank0,
            0,
            dsp::ParameterId::Gain),
        6.0F)}));
    (void)driver.Write(0, 4001, test::Entries({test::Write(
        test::FilterPath(
            instanceId,
            dsp::BankId::Bank1,
            0,
            dsp::ParameterId::Gain),
        3.0F)}));
    const auto bank0 = AwaitEqualizerResponse(
        [](const auto& snapshot)
        {
            return std::abs(snapshot.filters[0].magnitudeDb[0] - 6.0F) < 0.05F &&
                std::abs(snapshot.combined.magnitudeDb[0] - 6.0F) < 0.05F &&
                std::abs(snapshot.allBanksCombined.magnitudeDb[0] - 9.0F) < 0.05F;
        });
    analysis::EqualizerCurveSnapshot repeatedCurve;
    analysis::AnalysisView resultView;
    EXPECT_TRUE(
        analysis::AnalysisService::Get().TryReadLatestCurve(
            repeatedCurve, 0, resultView));
    EXPECT_EQ(repeatedCurve.revision, bank0.revision);
    EXPECT_FALSE(
        analysis::AnalysisService::Get().TryReadLatestCurve(
            repeatedCurve, bank0.revision, resultView));

    analysis::AnalysisService::Get().SetView(
        {instanceId, dsp::BankId::Bank1});
    const auto bank1 = AwaitEqualizerResponse(
        [&bank0](const auto& snapshot)
        {
            return snapshot.revision > bank0.revision &&
                snapshot.viewRevision > bank0.viewRevision &&
                snapshot.combined.sourceRevision ==
                    bank0.combined.sourceRevision &&
                std::abs(snapshot.filters[0].magnitudeDb[0] - 3.0F) < 0.05F &&
                std::abs(snapshot.combined.magnitudeDb[0] - 3.0F) < 0.05F &&
                std::abs(snapshot.allBanksCombined.magnitudeDb[0] - 9.0F) < 0.05F;
        });

    EXPECT_TRUE(bank1.revision > bank0.revision);
}

TEST_CASE("Switching spectrum instances discards a partial FFT window")
{
    test::ProtocolDriver driver{2};
    auto& first = driver.At(0);
    auto& second = driver.At(1);
    first.Prepare(48000.0);
    second.Prepare(48000.0);

    analysis::AnalysisService::Get().SetView(
        {first.GetInstanceId(), dsp::BankId::Bank0});
    driver.MainInput().fill(1.0);
    for (std::size_t block = 0; block < analysis::kFftSize / 32; ++block)
    {
        driver.ProcessAll();
    }

    analysis::AnalysisService::Get().SetView(
        {second.GetInstanceId(), dsp::BankId::Bank0});
    analysis::AnalysisService::Get().SetView(
        {first.GetInstanceId(), dsp::BankId::Bank0});
    for (std::size_t block = 0; block < analysis::kFftSize / 32; ++block)
    {
        driver.ProcessAll();
    }

    analysis::SpectrumSnapshot snapshot;
    analysis::AnalysisView resultView;
    EXPECT_FALSE(analysis::AnalysisService::Get().TryReadLatestSpectrum(
        snapshot, 0, resultView));

    for (std::size_t block = 0; block < analysis::kFftSize / 32; ++block)
    {
        driver.ProcessAll();
    }

    bool received = false;
    for (std::size_t attempt = 0; attempt < 500 && !received; ++attempt)
    {
        received = analysis::AnalysisService::Get().TryReadLatestSpectrum(
            snapshot, 0, resultView);
        if (!received)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{2});
        }
    }
    EXPECT_TRUE(received);
}

TEST_CASE("Current analysis view publishes revision-aware DSP telemetry")
{
    test::ProtocolDriver driver{2};
    auto& viewed = driver.At(0);
    auto& hidden = driver.At(1);
    viewed.Prepare(48000.0);
    hidden.Prepare(48000.0);
    analysis::AnalysisService::Get().SetView(
        {viewed.GetInstanceId(), dsp::BankId::Bank0});
    driver.MainInput().fill(0.5);

    driver.ProcessAll();

    dsp::TelemetrySnapshot snapshot;
    analysis::AnalysisView resultView;
    EXPECT_TRUE(analysis::AnalysisService::Get().TryReadLatestTelemetry(
        snapshot, 0, 0, resultView));
    EXPECT_TRUE(snapshot.revision > 0);
    EXPECT_TRUE(snapshot.viewRevision > 0);
    EXPECT_EQ(resultView.instanceId, viewed.GetInstanceId());
    EXPECT_EQ(resultView.bankId, dsp::BankId::Bank0);
    EXPECT_NEAR(
        snapshot.levels[dsp::ToIndex(dsp::MeterPoint::InputGainOutput)].rmsDb,
        -6.0206,
        0.01);
    EXPECT_FALSE(analysis::AnalysisService::Get().TryReadLatestTelemetry(
        snapshot, snapshot.revision, snapshot.viewRevision, resultView));

    const auto previousRevision = snapshot.revision;
    const auto previousViewRevision = snapshot.viewRevision;
    analysis::AnalysisService::Get().SetView(
        {hidden.GetInstanceId(), dsp::BankId::Bank0});
    driver.ProcessAll();

    EXPECT_TRUE(analysis::AnalysisService::Get().TryReadLatestTelemetry(
        snapshot, previousRevision, previousViewRevision, resultView));
    EXPECT_TRUE(snapshot.viewRevision > previousViewRevision);
    EXPECT_EQ(resultView.instanceId, hidden.GetInstanceId());
}

TEST_MAIN()
