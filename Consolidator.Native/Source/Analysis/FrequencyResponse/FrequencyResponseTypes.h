#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace consolidator::analysis
{

inline constexpr std::size_t kResponsePointCount = 256;
inline constexpr std::size_t kMaximumResponseStages = 64;

struct BiquadResponseSection
{
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
};

struct FrequencyResponseRequest
{
    std::uint64_t revision = 0;
    double sampleRate = 0.0;
    std::array<BiquadResponseSection, kMaximumResponseStages> stages{};
    std::size_t stageCount = 0;
};

struct FrequencyResponseSnapshot
{
    std::array<float, kResponsePointCount> magnitudeDb{};
    std::uint64_t sourceRevision = 0;
    std::uint64_t revision = 0;
    std::uint64_t viewRevision = 0;
};

struct EqualizerCurveSnapshot
{
    std::array<FrequencyResponseSnapshot, 7> filters{};
    FrequencyResponseSnapshot combined;
    FrequencyResponseSnapshot allBanksCombined;
    std::uint64_t revision = 0;
    std::uint64_t viewRevision = 0;
};

struct DetectorCurveSnapshot
{
    std::array<FrequencyResponseSnapshot, 2> filters{};
    std::array<bool, 2> filterActive{};
    FrequencyResponseSnapshot combined;
    bool active = false;
    std::uint64_t revision = 0;
    std::uint64_t viewRevision = 0;
};

struct DetectorCurvesSnapshot
{
    DetectorCurveSnapshot compressor;
    DetectorCurveSnapshot saturator;
    std::uint64_t revision = 0;
    std::uint64_t viewRevision = 0;
};

struct AnalysisCurveSnapshot
{
    EqualizerCurveSnapshot equalizer;
    DetectorCurvesSnapshot detectors;
    std::uint64_t revision = 0;
    std::uint64_t viewRevision = 0;
};

} // namespace consolidator::analysis
