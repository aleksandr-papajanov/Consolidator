#include "Core/Domain/State/StateStore.h"
#include "Core/Settings/DspDeviceSettings.h"

namespace consolidator::core
{

ChainState MakeChainState(const core::settings::DspSettings& settings);

StateStore::StateStore() noexcept
    : chain_(MakeChainState(core::settings::DspSettings{}))
{
}

} // namespace consolidator::core
