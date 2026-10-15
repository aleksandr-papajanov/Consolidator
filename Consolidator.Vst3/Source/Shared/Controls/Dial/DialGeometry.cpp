#include "Shared/Controls/Dial/DialGeometry.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace consolidator::vst3::shared::controls::dial
{

DialGeometry DialGeometry::FromBounds(juce::Rectangle<int> bounds)
{
    const auto width = static_cast<float>(bounds.getWidth());
    const auto height = static_cast<float>(bounds.getHeight());
    return { { bounds.getX() + width * 0.5F, bounds.getY() + height * 0.55F },
        std::max(1.0F, std::min(width, height) * 0.38F),
        { bounds.getX(), bounds.getBottom() - 17, bounds.getWidth(), 14 } };
}

float DialGeometry::Angle(double position)
{
    return static_cast<float>(std::numbers::pi * (4.0 / 3 + 4.0 / 3 * position));
}

juce::Point<float> DialGeometry::Point(double position) const
{
    const auto angle = Angle(position);
    return { center.x + std::sin(angle) * radius, center.y - std::cos(angle) * radius };
}

} // namespace consolidator::vst3::shared::controls::dial
