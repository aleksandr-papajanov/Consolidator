#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace emulation_lab
{

struct ProbeContext
{
    double sampleRate{};
    std::size_t blockSize{};
    unsigned int seed{};
};

enum class FeatureComparison
{
    Linear,
    WrappedDegrees
};

struct ProbeFeature
{
    std::string identifier;
    std::string group;
    std::string unit;
    double value{};
    double scale{ 1.0 };
    double floor{ -std::numeric_limits<double>::infinity() };
    FeatureComparison comparison{ FeatureComparison::Linear };
};

struct ProbeResult
{
    std::string probeId;
    std::string probeVersion;
    std::vector<ProbeFeature> features;
};

struct StereoSignal
{
    std::vector<double> left;
    std::vector<double> right;
};

using ProcessAudio = std::function<std::vector<double>(std::span<const double> input)>;

struct ProcessorCallbacks
{
    ProcessAudio process;
    std::function<void()> reset;
    std::function<std::size_t()> latencySamples;
    std::function<StereoSignal(
        std::span<const double> left,
        std::span<const double> right)> processStereo;
};

struct ToneCase
{
    double frequency{};
    double levelDb{};
};

struct SteppedSineConfiguration
{
    double minimumFrequency{ 20.0 };
    double maximumFrequency{ 20000.0 };
    double pointsPerOctave{ 12.0 };
    double minimumLevelDb{ -30.0 };
    double maximumLevelDb{ 10.0 };
    double levelStepDb{ 1.0 };
    double minimumPreRollSeconds{ 0.5 };
    double minimumMeasurementSeconds{ 0.5 };
    double preRollCycles{ 12.0 };
    double measurementCycles{ 20.0 };
    double harmonicFloorDbc{ -180.0 };
    std::size_t maximumHarmonic{ 16 };
    bool resetEachPoint{ true };
};

class SteppedSineProbe final
{
public:
    explicit SteppedSineProbe(SteppedSineConfiguration configuration = {});

    [[nodiscard]] std::vector<ToneCase> Cases() const;
    [[nodiscard]] std::vector<ProbeResult> Run(
        const ProbeContext& context,
        const ProcessorCallbacks& processor) const;
    [[nodiscard]] ProbeResult RunTone(
        const ProbeContext& context,
        const ProcessorCallbacks& processor,
        const ToneCase& tone) const;

private:
    SteppedSineConfiguration configuration_;
};

struct LevelSweepConfiguration
{
    double frequency{ 100.0 };
    std::vector<double> levelsDb{
        -36.0, -30.0, -24.0, -18.0, -12.0, -6.0, 0.0, 6.0
    };
    double secondsPerLevel{ 0.5 };
    double harmonicFloorDbc{ -180.0 };
    std::size_t maximumHarmonic{ 8 };
};

class LevelSweepProbe final
{
public:
    explicit LevelSweepProbe(LevelSweepConfiguration configuration = {});
    [[nodiscard]] std::vector<ProbeResult> Run(
        const ProbeContext& context,
        const ProcessorCallbacks& processor) const;

private:
    LevelSweepConfiguration configuration_;
};

struct TransientBurstConfiguration
{
    std::vector<double> frequencies{ 60.0, 200.0, 1000.0, 5000.0 };
    std::vector<double> levelsDb{ -18.0, -6.0, 0.0 };
    double silenceSeconds{ 0.25 };
    double sustainSeconds{ 0.5 };
    double tailSeconds{ 0.5 };
    double envelopeWindowSeconds{ 0.005 };
};

class TransientBurstProbe final
{
public:
    explicit TransientBurstProbe(TransientBurstConfiguration configuration = {});
    [[nodiscard]] ProbeResult Run(
        const ProbeContext& context,
        const ProcessorCallbacks& processor,
        double frequency,
        double levelDb) const;

private:
    TransientBurstConfiguration configuration_;
};

struct MultitoneConfiguration
{
    std::vector<double> frequencies{ 997.0, 1201.0, 1511.0 };
    double levelDb{ -18.0 };
    std::size_t samples{ 65536 };
};

class MultitoneProbe final
{
public:
    explicit MultitoneProbe(MultitoneConfiguration configuration = {});
    [[nodiscard]] ProbeResult Run(
        const ProbeContext& context,
        const ProcessorCallbacks& processor) const;

private:
    MultitoneConfiguration configuration_;
};

struct StereoInteractionConfiguration
{
    double frequency{ 1000.0 };
    double levelDb{ -12.0 };
    double seconds{ 0.5 };
};

class StereoInteractionProbe final
{
public:
    explicit StereoInteractionProbe(StereoInteractionConfiguration configuration = {});
    [[nodiscard]] ProbeResult Run(
        const ProbeContext& context,
        const ProcessorCallbacks& processor) const;

private:
    StereoInteractionConfiguration configuration_;
};


} // namespace emulation_lab
