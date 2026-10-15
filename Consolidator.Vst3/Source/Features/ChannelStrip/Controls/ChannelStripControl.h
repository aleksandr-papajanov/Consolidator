#pragma once

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

#include <JuceHeader.h>

#include "Shared/Controls/Parameter/ParameterControl.h"
#include "Features/ChannelStrip/Bindings/ChannelStripBindings.h"
#include "Shared/Controls/Parameter/Presentation/ParameterPresentation.h"
#include "Shared/Controls/ActionButton.h"

namespace consolidator::vst3::features::channelstrip::controls
{

class ChannelStripControl final : public juce::Component
{
public:
    ChannelStripControl();
    void Clear();
    void Rebuild(bindings::ChannelStripBindings& bindings);
    void paint(juce::Graphics& graphics) override;
    void RefreshPath(const std::string& path);
    void SetScopeColor(juce::Colour color);
    void SetEditing(bool enabled, bool navigation);
    void FinishPreview(bool accepted);
    void resized() override;

private:
    void Replace(std::vector<std::unique_ptr<shared::controls::parameter::ParameterControl>> controls,
        std::span<const shared::controls::parameter::presentation::ParameterPresentation> presentations);
    std::vector<std::unique_ptr<shared::controls::parameter::ParameterControl>> controls_;
    std::vector<std::pair<int, int>> positions_;
    shared::controls::ActionButton gainStagingReset_{ "R" };
};

} // namespace consolidator::vst3::features::channelstrip::controls
