#include "Features/Equalizer/Controllers/EqualizerController.h"

#include <cstdint>
#include <utility>

namespace consolidator::vst3::features::equalizer::controllers
{

EqualizerController::EqualizerController(shared::clients::ProtocolClient& commands) : commands_(commands)
{
}

void EqualizerController::Process(const std::string& selector, const shared::clients::UiTarget& target,
    bool groupScope, shared::clients::ProtocolClient::Completion completion)
{
    // Optimization handlers own their history advancement.
    commands_.Send(selector, { std::to_string(target.instance), static_cast<std::int64_t>(target.bank),
        std::string(groupScope ? "group" : "local") }, std::move(completion));
}

} // namespace consolidator::vst3::features::equalizer::controllers
