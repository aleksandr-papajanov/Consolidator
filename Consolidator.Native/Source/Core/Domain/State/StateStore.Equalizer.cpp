#include "Core/Domain/State/StateStore.Internal.h"

namespace consolidator::core
{

ApplyResult StateStore::WriteEqualizerState(
    const StatePath& path,
    const StateValue& value)
{
    if (path.depth == 0)
    {
        return ApplyResult::NotHandled;
    }

    const auto bankIndex = static_cast<std::size_t>(path.nodes[0]) -
        static_cast<std::size_t>(dsp::RouteNodeId::Bank0);
    if (bankIndex >= chain_.equalizers.size())
    {
        return ApplyResult::NotHandled;
    }

    if (path.depth == 1)
    {
        return detail::ApplyParameter(
            path,
            value,
            chain_.equalizers[bankIndex].bypass);
    }

    const auto filterIndex = static_cast<std::size_t>(path.nodes[1]) -
        static_cast<std::size_t>(dsp::RouteNodeId::Filter1);
    if (filterIndex >= chain_.equalizerFilters[bankIndex].size())
    {
        return ApplyResult::NotHandled;
    }
    return WriteFilterState(
        path,
        value,
        chain_.equalizerFilters[bankIndex][filterIndex]);
}

} // namespace consolidator::core
