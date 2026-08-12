#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <cstdint>
#include <vector>

#include "Dsp/Processors/DspDevice.h"
#include "Dsp/Telemetry/Telemetry.h"
#include "Dsp/Telemetry/MeterSmoother.h"
#include "Dsp/Telemetry/PeakMeter.h"
#include "Core/Instance/Queues/RuntimeUpdateMailbox.h"

namespace consolidator::core
{
}

namespace consolidator::dsp
{

class Compressor;
class Saturator;

// Owns the ordered DSP devices and processes audio through the complete chain.
class DspChain final
{
public:
    DspChain() = default;
    ~DspChain() = default;

    DspChain(const DspChain&) = delete;
    DspChain& operator=(const DspChain&) = delete;

    void AddDevice(std::unique_ptr<DspDevice> device);
    // Enables block telemetry collection for the currently viewed instance.
    void SetTelemetryEnabled(bool enabled) noexcept;
    // Registers fixed processor telemetry owners during chain construction.
    void SetTelemetryProcessors(
        Compressor* compressor,
        Saturator* saturator) noexcept;
    // Prepares every device before the audio callback starts.
    void Prepare(
        double sampleRate,
        std::size_t channelCount);
    // Applies one coalesced mailbox batch before processing the next audio block.
    void ApplyRuntimeUpdates(const core::ParameterUpdateBatch& batch);
    // Applies active/listen runtime controls after parameters and before Process.
    void ApplyRuntimeControlUpdates(const core::RuntimeControlBatch& batch);
    // Resets runtime memory for devices matching the requested route.
    void Reset(const core::StatePath& target) noexcept;

    // Runs all active devices using preallocated intermediate buffers.
    void Process(const double* inputLeft,
                 const double* inputRight,
                 double* interimLeft,
                 double* interimRight,
                 double* outputLeft,
                 double* outputRight,
                 std::size_t frameCount);

    // Completes the current block's meters and returns the latest snapshot.
    [[nodiscard]] TelemetrySnapshot FinishTelemetryBlock(
        std::size_t frameCount) noexcept;

    [[nodiscard]] std::size_t GetDeviceCount() const noexcept;

    [[nodiscard]] DspDevice* GetDevice(std::size_t index) noexcept;
    [[nodiscard]] const DspDevice* GetDevice(std::size_t index) const noexcept;

private:
    static constexpr std::size_t kMaximumDevices = 64;

    std::vector<std::unique_ptr<DspDevice>> devices_;
    Compressor* compressor_ = nullptr;
    Saturator* saturator_ = nullptr;
    struct LevelAccumulator
    {
        double sumSquares = 0.0;
        double peak = 0.0;
        std::size_t sampleCount = 0;
    };

    std::array<LevelAccumulator, ToIndex(MeterPoint::Count)> levelAccumulators_{};
    std::array<MeterSmoother, ToIndex(MeterPoint::Count)> levelSmoothers_{
        MeterSmoother{0.0f}, MeterSmoother{0.0f},
        MeterSmoother{0.0f}, MeterSmoother{0.0f}};
    std::array<PeakMeter, ToIndex(MeterPoint::Count)> levelPeakMeters_{};
    MeterSmoother compressorGainReductionSmoother_{0.0f};
    PeakMeter compressorGainReductionPeakMeter_;
    MeterSmoother distortionSmoother_;
    std::uint64_t telemetryRevision_ = 0;
    bool telemetryEnabled_ = true;
};

} // namespace consolidator::dsp
