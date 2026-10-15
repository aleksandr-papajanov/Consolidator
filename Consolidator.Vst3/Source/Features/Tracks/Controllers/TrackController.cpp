#include "Features/Tracks/Controllers/TrackController.h"

#include <stdexcept>
#include <utility>

namespace consolidator::vst3::features::tracks::controllers
{

TrackController::TrackController(shared::clients::ProtocolClient& commands,
    const shared::clients::RegistryClient& registry)
    : commands_(commands), registry_(registry)
{
}

void TrackController::Execute(const features::tracks::bindings::TrackIntent& intent,
    const shared::clients::UiTarget& focus, bool groupScope, Completion completion)
{
    const auto target = std::to_string(intent.instance);
    const auto value = std::int64_t{ intent.value ? 1 : 0 };
    const std::string scope = groupScope ? "group" : "local";
    if (intent.kind == features::tracks::bindings::TrackIntentKind::SetMute ||
        intent.kind == features::tracks::bindings::TrackIntentKind::SetSolo ||
        intent.kind == features::tracks::bindings::TrackIntentKind::SetInstanceBypass)
    {
        const auto selector = intent.kind == features::tracks::bindings::TrackIntentKind::SetMute
            ? "set_instance_mute"
            : intent.kind == features::tracks::bindings::TrackIntentKind::SetSolo
            ? "set_instance_solo" : "set_instance_bypass";
        const auto bank = intent.instance == focus.instance ? registry_.FindBank(intent.instance, focus.bank) : nullptr;
        const std::string editScope = intent.additive && (bank == nullptr || !bank->group) ? "local" : scope;
        commands_.Send(selector, { target, editScope, value,
            std::string(intent.additive ? "additive" : "exclusive") }, std::move(completion));
    }
    else if (intent.kind == features::tracks::bindings::TrackIntentKind::SetBankBypass)
    {
        commands_.Send("set_bank_bypass", { target, static_cast<std::int64_t>(intent.bank), scope, value },
            std::move(completion));
    }
    else if (intent.kind == features::tracks::bindings::TrackIntentKind::ResetBank ||
        intent.kind == features::tracks::bindings::TrackIntentKind::ResetInstance)
    {
        const auto bankReset = intent.kind == features::tracks::bindings::TrackIntentKind::ResetBank;
        const auto instanceReset = intent.kind == features::tracks::bindings::TrackIntentKind::ResetInstance;
        const auto parts = shared::protocol::EncodePath(bankReset ? "equalizer.bank" : "dsp");
        std::vector<shared::protocol::ProtocolValue> body;
        if (instanceReset)
        {
            // The original table instance Reset resets the current UI target.
            body = { std::string("0"), std::string(groupScope ? "group_instance" : "local") };
        }
        else
        {
            body = { std::string("target"), target,
                static_cast<std::int64_t>(intent.bank), std::string("0"), scope };
        }
        body.insert(body.end(), parts.begin(), parts.end());
        commands_.Send("reset", std::move(body), std::move(completion));
    }
    else
    {
        throw std::invalid_argument("Unsupported track action.");
    }
}

void TrackController::Group(Banks banks, Completion completion)
{
    std::vector<shared::protocol::ProtocolValue> body{ static_cast<std::int64_t>(banks.size()) };
    for (const auto& bank : banks)
    {
        body.emplace_back(std::to_string(bank.first));
        body.emplace_back(static_cast<std::int64_t>(bank.second));
    }
    commands_.Send("group_banks", std::move(body), std::move(completion));
}

void TrackController::Ungroup(const shared::clients::UiTarget& target, Completion completion)
{
    commands_.Send("ungroup_banks", { std::to_string(target.instance), static_cast<std::int64_t>(target.bank) },
        std::move(completion));
}

void TrackController::Clear(Completion completion)
{
    commands_.Send("clear_topology", {}, std::move(completion));
}

} // namespace consolidator::vst3::features::tracks::controllers
