#pragma once

#include "DspExecution.h"
#include "DspParallelStage.h"
#include "DspProcessors.h"

namespace consolidator::max::dsp
{

class DspRuntimeGraph final
{
public:
    void Prepare(double sampleRate, std::size_t maximumFrameCount);
    void Reset() noexcept;
    void SetParameters(const DspSnapshot& snapshot) noexcept;
    void SetProcessingPlan(const DspProcessingPlan& plan) noexcept;
    void ProcessBlock(
        double* mainLeft,
        double* mainRight,
        std::size_t frameCount) noexcept;

    [[nodiscard]] const DspProcessingContext& Context() const noexcept;
    [[nodiscard]] const DspProcessingPlan& ProcessingPlan() const noexcept;
    [[nodiscard]] bool HasInputLevelTelemetry() const noexcept;
    [[nodiscard]] float InputLevelTelemetry() const noexcept;
    [[nodiscard]] float FastInputLevelTelemetry() const noexcept;

private:
    DspProcessingContext context_{};
    DspProcessingPlan plan_{};
    DspExecutionPolicy policy_{};
    DspSnapshot snapshot_{};
    InputProcessor input_;
    SaturatorProcessor saturator_;
    CompressorProcessor compressor_;
    DspParallelStage compressorParallel_;
    EqualizerProcessor equalizer_;
    PolishProcessor polish_;
    OutputProcessor output_;
};

} // namespace consolidator::max::dsp
