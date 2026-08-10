#include "Core/Domain/State/StateStore.Internal.h"

namespace consolidator::core
{

ApplyResult StateStore::WriteCompressorState(
    const StatePath& path,
    const StateValue& value)
{
    if (path.depth == 0)
    {
        auto result = detail::ApplyParameter(path, value, chain_.compressor.thresholdDb);
        if (result == ApplyResult::NotHandled)
        {
            result = detail::ApplyParameter(path, value, chain_.compressor.ratio);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = detail::ApplyParameter(path, value, chain_.compressor.attackMs);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = detail::ApplyParameter(path, value, chain_.compressor.releaseMs);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = detail::ApplyParameter(path, value, chain_.compressor.outputDb);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = detail::ApplyParameter(path, value, chain_.compressor.mix);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = detail::ApplyParameter(path, value, chain_.compressor.bypass);
        }
        return result;
    }

    if (path.nodes[0] != dsp::RouteNodeId::Detector || path.depth <= 1)
    {
        return ApplyResult::NotHandled;
    }

    const auto filterIndex = static_cast<std::size_t>(path.nodes[1]) -
        static_cast<std::size_t>(dsp::RouteNodeId::Filter1);
    if (filterIndex >= chain_.compressorDetectorFilters.size())
    {
        return ApplyResult::NotHandled;
    }
    return WriteFilterState(
        path,
        value,
        chain_.compressorDetectorFilters[filterIndex]);
}

} // namespace consolidator::core
