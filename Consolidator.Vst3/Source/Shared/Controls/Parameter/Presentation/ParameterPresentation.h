#pragma once

#include <span>
#include <string_view>

namespace consolidator::vst3::shared::controls::parameter::presentation
{

struct ParameterPresentation
{
    std::string_view context;
    std::span<const std::string_view> orderedPaths;
    std::string_view choicePath;
    std::span<const std::string_view> choiceLabels;

    [[nodiscard]] std::span<const std::string_view> ChoicesFor(std::string_view path) const
    {
        return path == choicePath ? choiceLabels : std::span<const std::string_view>{};
    }
};

} // namespace consolidator::vst3::shared::controls::parameter::presentation
