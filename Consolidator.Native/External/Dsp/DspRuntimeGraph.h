#pragma once

#include "DspParallelStage.h"
#include "DspExecution.h"
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
        const double* sidechainLeft,
        const double* sidechainRight,
        std::size_t frameCount) noexcept;

    [[nodiscard]] const DspProcessingContext& Context() const noexcept;
    [[nodiscard]] const DspProcessingPlan& ProcessingPlan() const noexcept;

private:
    DspProcessingContext context_{};
    DspProcessingPlan plan_{};
    DspExecutionPolicy policy_{};
    DspSnapshot snapshot_{};
    InputProcessor input_;
    EqualizerDetectorProcessor equalizerDetector_;
    LevelerProcessor leveler_;
    SaturatorProcessor saturator_;
    CompressorProcessor compressor_;
    EqualizerProcessor equalizer_;
    DspParallelStage compressorParallel_;
    DspParallelStage polishParallel_;
    PolishProcessor polish_;
    OutputProcessor output_;
    LimiterProcessor limiter_;
};

} // namespace consolidator::max::dsp
