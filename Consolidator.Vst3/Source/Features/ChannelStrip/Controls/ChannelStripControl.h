#pragma once

#include <memory>
#include <string>
#include <vector>

#include <JuceHeader.h>

#include "Features/ChannelStrip/Bindings/ChannelStripBindings.h"
#include "Shared/Controls/MarkerControl.h"
#include "Shared/Controls/Parameter/ParameterControl.h"
#include "Shared/Clients/RegistryClient.h"

namespace consolidator::vst3::features::channelstrip::controls
{

class ChannelStripControl final : public juce::Component
{
public:
    void Clear();
    void Rebuild(bindings::ChannelStripBindings& bindings);
    void paint(juce::Graphics& graphics) override;
    void RefreshPath(const std::string& path);
    void ApplyProcessorStatus(const shared::clients::RegistryProcessor& processor);
    void SetScopeColor(juce::Colour color);
    void SetEditing(bool enabled, bool navigation);
    void FinishPreview(bool accepted);
    void resized() override;

private:
    struct ProcessorHeader
    {
        // Borrowed from bindings; Clear destroys headers before models are replaced.
        shared::controls::parameter::viewmodels::ParameterViewModel* model;
        std::unique_ptr<shared::controls::MarkerControl> marker;
    };

    std::vector<std::unique_ptr<shared::controls::parameter::ParameterControl>> controls_;
    std::vector<ProcessorHeader> headers_;
};

} // namespace consolidator::vst3::features::channelstrip::controls
