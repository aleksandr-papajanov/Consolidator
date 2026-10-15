#include "Shared/Clients/StateCommands.h"

#include <utility>
#include <vector>

namespace consolidator::vst3::shared::clients
{

StateCommands::StateCommands(ProtocolClient& commands, const TargetStateClient& state)
    : commands_(commands), state_(state)
{
}

void StateCommands::WriteDiscrete(const std::string& path, std::int64_t value, bool groupScope,
    ProtocolClient::Completion completion)
{
    const auto& selected = state_.Target();
    const auto target = std::to_string(selected.instance);
    const std::string scope = groupScope ? "group" : "local";
    if (path.ends_with(".bypass") && path != "equalizer.bank.bypass" && path.find('.', path.find('.') + 1) == std::string::npos)
    {
        const auto device = path.substr(0, path.find('.'));
        const auto& context = device;
        commands_.Send("set_processor_bypass", { target, context, scope, value }, std::move(completion));
    }
    else if (path == "equalizer.bank.bypass")
    {
        commands_.Send("set_bank_bypass", { target, static_cast<std::int64_t>(selected.bank), scope, value },
            std::move(completion));
    }
    else
    {
        commands_.Send("write", shared::protocol::EncodeStateWrite(
            { { path, value, state_.WriteMode(path, groupScope) } }, scope, "0"), std::move(completion));
    }
}

void StateCommands::ResetValue(const std::string& path, bool groupScope, ProtocolClient::Completion completion)
{
    const std::string scope = !groupScope ? "local" : path == "dsp" ? "group_instance" : "group";
    std::vector<shared::protocol::ProtocolValue> body{ std::string("0"), scope };
    const auto resetPath = path.starts_with("equalizer.filter.")
        ? "equalizer.bank." + path.substr(std::string("equalizer.").size()) : path;
    const auto parts = shared::protocol::EncodePath(resetPath);
    body.insert(body.end(), parts.begin(), parts.end());
    commands_.Send("reset", std::move(body), std::move(completion));
}

} // namespace consolidator::vst3::shared::clients
