#include "Core/Domain/State/StateStore.Internal.h"

namespace consolidator::core
{

ApplyResult StateStore::WriteFilterState(
    const StatePath& path,
    const StateValue& value,
    dsp::FilterState& state) const
{
    auto result = detail::ApplyParameter(path, value, state.frequencyHz);
    if (result == ApplyResult::NotHandled)
    {
        result = detail::ApplyParameter(path, value, state.q);
    }
    if (result == ApplyResult::NotHandled)
    {
        result = detail::ApplyParameter(path, value, state.gainDb);
    }
    if (result == ApplyResult::NotHandled)
    {
        result = detail::ApplyParameter(path, value, state.bypass);
    }
    return result;
}

ApplyResult StateStore::WriteSaturatorState(
    const StatePath& path,
    const StateValue& value)
{
    if (path.depth == 0)
    {
        auto result = detail::ApplyParameter(path, value, chain_.saturator.drive);
        if (result == ApplyResult::NotHandled)
        {
            result = detail::ApplyParameter(path, value, chain_.saturator.outputDb);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = detail::ApplyParameter(path, value, chain_.saturator.mix);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = detail::ApplyParameter(path, value, chain_.saturator.detectorAmount);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = detail::ApplyParameter(path, value, chain_.saturator.bypass);
        }
        return result;
    }

    if (path.nodes[0] != dsp::RouteNodeId::Detector || path.depth <= 1)
    {
        return ApplyResult::NotHandled;
    }

    const auto filterIndex = static_cast<std::size_t>(path.nodes[1]) -
        static_cast<std::size_t>(dsp::RouteNodeId::Filter1);
    if (filterIndex >= chain_.saturatorDetectorFilters.size())
    {
        return ApplyResult::NotHandled;
    }
    return WriteFilterState(
        path,
        value,
        chain_.saturatorDetectorFilters[filterIndex]);
}

} // namespace consolidator::core
