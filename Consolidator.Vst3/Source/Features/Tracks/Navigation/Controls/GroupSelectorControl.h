#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>

#include <JuceHeader.h>

#include "Features/Tracks/ViewModels/TracksViewModel.h"
#include "Shared/Theme/UiColors.h"

namespace consolidator::vst3::features::tracks::navigation::controls
{

class GroupSelectorControl final : public juce::Component
{
public:
    GroupSelectorControl(const tracks::viewmodels::TracksViewModel& tracks,
        std::function<void(std::uint32_t)> select);
    void Refresh(std::optional<std::uint32_t> selected);
    [[nodiscard]] int PreferredWidth() const;
    void resized() override;

private:
    const tracks::viewmodels::TracksViewModel& tracks_;
    std::function<void(std::uint32_t)> select_;
    std::array<juce::TextButton, shared::ui::kGroups.size()> buttons_;
};

} // namespace consolidator::vst3::features::tracks::navigation::controls
