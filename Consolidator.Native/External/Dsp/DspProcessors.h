#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "DspProcessor.h"

namespace consolidator::max::dsp
{

class DspGainRamp final
{
public:
    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetTargetDb(float targetDb) noexcept;
    void Process(
        DspAudioBuffer& audio,
        DspChannelMode channelMode) noexcept;

    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    double currentGain_{ 1.0 };
    double targetGain_{ 1.0 };
    double gainRatio_{ 1.0 };
    std::size_t rampSampleCount_{ 1 };
    std::size_t remainingSamples_{};
};

class DetectorProcessor final : public DspProcessor
{
public:
    explicit DetectorProcessor(std::size_t filterOffset) noexcept;

    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    static constexpr std::size_t kFilterCount = 2;

    std::size_t filterOffset_{};
    std::array<FilterSnapshot, kFilterCount> filters_{};
    std::vector<double> left_;
    std::vector<double> right_;
};

class LevelerProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    float targetDb_{};
};

class InputProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    void ProcessWidth(DspProcessingFrame& frame) noexcept;

    DspGainRamp level_;
    DetectorProcessor levelerDetector_{ 0 };
    LevelerProcessor leveler_;
    float currentWidth_{ 100.0F };
    float targetWidth_{ 100.0F };
    float widthStep_{};
    std::size_t widthRampSampleCount_{ 1 };
    std::size_t widthRemainingSamples_{};
};

class SaturationProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    float drive_{};
    float curve_{ 0.5F };
    bool split_{};
};

class SaturatorProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;
    void SynchronizeInactiveState() noexcept;

private:
    DetectorProcessor detector_{ 2 };
    SaturationProcessor saturation_;
    DspGainRamp output_;
};

class TransientShaperProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    float attack_{};
    float sustain_{};
};

class CompressionProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    float amount_{};
    std::uint32_t character_{};
};

class CompressorProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;
    void SynchronizeInactiveState() noexcept;

private:
    DetectorProcessor detector_{ 4 };
    TransientShaperProcessor transientShaper_;
    CompressionProcessor compression_;
    DspGainRamp output_;
};

class EqualizerProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    std::array<std::uint32_t, 7> activeBanks_{};
    std::array<FilterSnapshot, 49> filters_{};
};

class PolishThickProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    float amount_{};
};

class PolishAirProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    float amount_{};
};

class PolishProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    PolishThickProcessor thick_;
    PolishAirProcessor air_;
    std::vector<double> thickLeft_;
    std::vector<double> thickRight_;
    std::vector<double> airLeft_;
    std::vector<double> airRight_;
};

class LimiterProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    float targetDb_{ -1.0F };
};

class OutputProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    DspGainRamp level_;
    LimiterProcessor limiter_;
    bool limiterEnabled_{};
};

} // namespace consolidator::max::dsp
