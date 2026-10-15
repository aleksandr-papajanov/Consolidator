#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Shared/Controls/Parameter/Presentation/ParameterPresentation.h"

namespace consolidator::vst3::features::channelstrip::presentation
{

enum class ParameterSlotKind
{
    Dial,
    Toolbar,
    Heading
};

// Centers are relative to the section; dial rows share the strip grid.
struct ParameterSlot
{
    std::string_view path;
    int center;
    int row;
    ParameterSlotKind kind{ ParameterSlotKind::Dial };
    std::string_view label{};
};

struct ProcessorSection
{
    shared::controls::parameter::presentation::ParameterPresentation parameters;
    int left;
    int width;
    std::span<const ParameterSlot> slots;
};

inline constexpr std::array<ParameterSlot, 9> kGainStagingSlots{
    ParameterSlot{ "gain_staging.input_leveler", 75, 0, ParameterSlotKind::Toolbar, "Level" },
    ParameterSlot{ "gain_staging.input_level", 75, 0 },
    ParameterSlot{ "gain_staging.input_target", 75, 1 },
    ParameterSlot{ "gain_staging.output_leveler", 225, 0, ParameterSlotKind::Toolbar, "Level" },
    ParameterSlot{ "gain_staging.output_level", 225, 0 },
    ParameterSlot{ "gain_staging.output_target", 225, 1 },
    ParameterSlot{ "gain_staging.limit", 225, 2 },
    ParameterSlot{ "gain_staging.width", 75, 2 },
    ParameterSlot{ "gain_staging.bypass", 150, 0, ParameterSlotKind::Heading }
};
inline constexpr std::array<ParameterSlot, 5> kSaturatorSlots{
    ParameterSlot{ "saturator.drive", 75, 0 },
    ParameterSlot{ "saturator.curve", 75, 1 },
    ParameterSlot{ "saturator.split", 75, 0, ParameterSlotKind::Toolbar },
    ParameterSlot{ "saturator.output", 75, 2 },
    ParameterSlot{ "saturator.bypass", 75, 0, ParameterSlotKind::Heading }
};
inline constexpr std::array<ParameterSlot, 4> kShaperSlots{
    ParameterSlot{ "shaper.bypass", 75, 0, ParameterSlotKind::Heading },
    ParameterSlot{ "shaper.attack", 75, 0 },
    ParameterSlot{ "shaper.sustain", 75, 1 },
    ParameterSlot{ "shaper.output", 75, 2 }
};
inline constexpr std::array<ParameterSlot, 8> kCompressionSlots{
    ParameterSlot{ "compression.bypass", 150, 0, ParameterSlotKind::Heading },
    ParameterSlot{ "compression.type", 75, 0 },
    ParameterSlot{ "compression.reduction", 75, 1 },
    ParameterSlot{ "compression.output", 225, 0 },
    ParameterSlot{ "compression.attack", 75, 2 },
    ParameterSlot{ "compression.release", 225, 1 },
    ParameterSlot{ "compression.ratio", 225, 2 },
    ParameterSlot{ "compression.parallel", 150, 0, ParameterSlotKind::Toolbar }
};
inline constexpr std::array<ParameterSlot, 3> kPolishSlots{
    ParameterSlot{ "polish.thick", 80, 0 },
    ParameterSlot{ "polish.air", 80, 1 },
    ParameterSlot{ "polish.bypass", 80, 0, ParameterSlotKind::Heading }
};

template<std::size_t Size>
constexpr std::array<std::string_view, Size> OrderedPaths(const std::array<ParameterSlot, Size>& slots)
{
    std::array<std::string_view, Size> paths{};
    for (std::size_t index = 0; index < Size; ++index)
    {
        paths[index] = slots[index].path;
    }
    return paths;
}

inline constexpr auto kGainStagingPaths = OrderedPaths(kGainStagingSlots);
inline constexpr auto kSaturatorPaths = OrderedPaths(kSaturatorSlots);
inline constexpr auto kShaperPaths = OrderedPaths(kShaperSlots);
inline constexpr auto kCompressionPaths = OrderedPaths(kCompressionSlots);
inline constexpr auto kPolishPaths = OrderedPaths(kPolishSlots);
inline constexpr std::array<std::string_view, 4> kCompressionTypeLabels{ "SSL", "76", "3A", "2A" };
inline constexpr std::array<ProcessorSection, 5> kSections{
    ProcessorSection{ { "gain_staging", kGainStagingPaths, {}, {} }, 20, 300, kGainStagingSlots },
    ProcessorSection{ { "saturator", kSaturatorPaths, {}, {} }, 320, 150, kSaturatorSlots },
    ProcessorSection{ { "shaper", kShaperPaths, {}, {} }, 470, 150, kShaperSlots },
    ProcessorSection{ { "compression", kCompressionPaths, "compression.type", kCompressionTypeLabels },
        620, 300, kCompressionSlots },
    ProcessorSection{ { "polish", kPolishPaths, {}, {} }, 920, 160, kPolishSlots }
};

inline constexpr auto kParameterSections = []
{
    std::array<shared::controls::parameter::presentation::ParameterPresentation, kSections.size()> parameters{};
    for (std::size_t index = 0; index < kSections.size(); ++index)
    {
        parameters[index] = kSections[index].parameters;
    }
    return parameters;
}();

inline const ProcessorSection& SectionFor(std::string_view context)
{
    for (const auto& section : kSections)
    {
        if (section.parameters.context == context)
        {
            return section;
        }
    }
    throw std::invalid_argument("Unknown channel strip section: " + std::string(context));
}

inline const ParameterSlot& SlotFor(const ProcessorSection& section, std::string_view path)
{
    for (const auto& slot : section.slots)
    {
        if (slot.path == path)
        {
            return slot;
        }
    }
    throw std::invalid_argument("Parameter has no channel strip slot: " + std::string(path));
}

} // namespace consolidator::vst3::features::channelstrip::presentation
