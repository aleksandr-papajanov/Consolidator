#pragma once

#include "DetectorEqualizer.h"
#include "WetTransition.h"
#include "../Algorithms/Cla2a/Cla2aProcessor.h"
#include "../Algorithms/Cla3a/Cla3aProcessor.h"
#include "../Algorithms/Cla76/Cla76Processor.h"
#include "../Algorithms/SslChannel/SslChannelProcessor.h"

namespace consolidator::audio::devices
{

// Values match the Managed Compression type contract.
enum class CompressorAlgorithm
{
    Ssl,
    Cla76,
    Cla3a,
    Cla2a
};

struct CompressorParameters
{
    CompressorAlgorithm algorithm{ CompressorAlgorithm::Ssl };
    double reduction{};
    double outputDb{};
    double attack{};
    double release{};
    double ratio{};
    bool parallel{};
    bool bypass{};
    std::array<FilterParameters, 2> detector{};
};

class CompressorDevice final
{
public:
    void Prepare(const ProcessingContext& context);
    void Reset() noexcept;
    void SetParameters(const CompressorParameters& parameters) noexcept;
    void Process(AudioBuffer audio, ChannelMode mode) noexcept;
    [[nodiscard]] double ReductionDb() const noexcept;

private:
    void ResetModels() noexcept;
    void ProcessWet(AudioBuffer audio, ChannelMode mode) noexcept;

    DetectorEqualizer detector_;
    algorithms::Cla76Processor cla76_;
    algorithms::Cla3aProcessor cla3a_;
    algorithms::Cla2aProcessor cla2a_;
    algorithms::SslChannelProcessor ssl_;
    WetTransition transition_;
    CompressorAlgorithm algorithm_{ CompressorAlgorithm::Ssl };
    double reductionDb_{};
    bool resetPending_{};
    bool bypass_{};
};

} // namespace consolidator::audio::devices
