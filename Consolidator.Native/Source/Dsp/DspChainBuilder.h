#pragma once

#include <memory>

#include "Core/Settings/DspDeviceSettings.h"

namespace consolidator::dsp
{

class DspChain;

class DspChainBuilder
{
public:
    DspChainBuilder() = default;
    ~DspChainBuilder() = default;

    DspChainBuilder(const DspChainBuilder&) = delete;
    DspChainBuilder& operator=(const DspChainBuilder&) = delete;

    std::unique_ptr<DspChain> BuildStandardChain() const;
    std::unique_ptr<DspChain> BuildFromSettings(const core::settings::DspSettings& settings) const;
};

} // namespace consolidator::dsp
