#include "Core/Domain/State/StateStore.Internal.h"

namespace consolidator::core
{

ApplyResult StateStore::WriteGainState(
    const StatePath& path,
    const StateValue& value,
    dsp::GainState& state) const
{
    auto result = detail::ApplyParameter(path, value, state.gainDb);
    if (result == ApplyResult::NotHandled)
    {
        result = detail::ApplyParameter(path, value, state.bypass);
    }
    return result;
}

} // namespace consolidator::core
