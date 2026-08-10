#pragma once

#include <memory>

#include "Core/Settings/DspDeviceSettings.h"

namespace consolidator::dsp
{

class DspChain;

// Constructs the standard device topology from centralized DSP settings.
class DspChainBuilder
{
public:
    DspChainBuilder() = default;
    ~DspChainBuilder() = default;

    DspChainBuilder(const DspChainBuilder&) = delete;
    DspChainBuilder& operator=(const DspChainBuilder&) = delete;

    std::unique_ptr<DspChain> BuildStandardChain() const;
    // Creates a chain whose devices and filters match the supplied settings.
    std::unique_ptr<DspChain> BuildFromSettings(const core::settings::DspSettings& settings) const;
};

} // namespace consolidator::dsp
