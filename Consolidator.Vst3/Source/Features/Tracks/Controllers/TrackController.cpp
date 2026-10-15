#include "Features/Tracks/Controllers/TrackController.h"

#include <utility>

#include "Shared/Clients/StateCommands.h"

namespace consolidator::vst3::features::tracks::controllers
{

TrackController::TrackController(shared::clients::ProtocolClient& commands,
    const shared::clients::RegistryClient& registry, shared::editing::HistoryActionSession& history)
    : commands_(commands), registry_(registry), history_(history),
      topology_([this](const std::string& selector, std::vector<shared::protocol::ProtocolValue> body, Completion completion)
          { commands_.Send(selector, std::move(body), std::move(completion)); })
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
    else if (intent.kind == features::tracks::bindings::TrackIntentKind::SetProcessorBypass)
    {
        commands_.Send("set_processor_bypass", { target, intent.processor, scope, value }, std::move(completion));
    }
    else if (intent.kind == features::tracks::bindings::TrackIntentKind::SetBankBypass)
    {
        commands_.Send("set_bank_bypass", { target, static_cast<std::int64_t>(intent.bank), scope, value },
            std::move(completion));
    }
    else
    {
        const auto bankReset = intent.kind == features::tracks::bindings::TrackIntentKind::ResetBank;
        const auto instanceReset = intent.kind == features::tracks::bindings::TrackIntentKind::ResetInstance;
        const auto parts = shared::protocol::EncodePath(bankReset ? "equalizer.bank" : instanceReset ? "dsp" :
            shared::clients::StateCommands::DevicePath(intent.processor));
        std::vector<shared::protocol::ProtocolValue> body;
        if (instanceReset)
        {
            // The original table instance Reset resets the current UI target.
            body = { std::string("0"), std::string(groupScope ? "group_instance" : "local") };
        }
        else
        {
            body = { std::string("target"), target,
                bankReset ? shared::protocol::ProtocolValue(static_cast<std::int64_t>(intent.bank)) :
                    shared::protocol::ProtocolValue(std::string("none")), std::string("0"), scope };
        }
        body.insert(body.end(), parts.begin(), parts.end());
        commands_.Send("reset", std::move(body), std::move(completion));
    }
}

void TrackController::Group(Banks banks, Completion completion)
{
    const auto group = registry_.NextGroup();
    if (!group || banks.size() < 2)
    {
        completion(false);
        return;
    }
    history_.Run([this, banks = std::move(banks), group](const std::string&, Completion done)
    {
        topology_.Write(banks, static_cast<std::int64_t>(*group), std::move(done));
    }, std::move(completion));
}

void TrackController::Ungroup(const shared::clients::UiTarget& target, Completion completion)
{
    const auto bank = registry_.FindBank(target.instance, target.bank);
    if (bank == nullptr || !bank->group || *bank->group == 0)
    {
        completion(false);
        return;
    }
    Banks banks;
    for (const auto& instance : registry_.Instances())
    {
        for (const auto& member : instance.banks)
        {
            if (member.group == bank->group)
            {
                banks.emplace_back(instance.id, member.index);
            }
        }
    }
    history_.Run([this, banks = std::move(banks)](const std::string&, Completion done)
    {
        topology_.Write(banks, std::string("none"), std::move(done));
    }, std::move(completion));
}

void TrackController::Clear(Completion completion)
{
    history_.Run([this](const std::string&, Completion done)
    {
        commands_.Send("clear_topology", {}, std::move(done));
    }, std::move(completion));
}

void TrackController::Process(const std::string& selector, const shared::clients::UiTarget& target,
    bool groupScope, Completion completion)
{
    // Optimization handlers own their history advancement.
    commands_.Send(selector, { std::to_string(target.instance), static_cast<std::int64_t>(target.bank),
        std::string(groupScope ? "group" : "local") }, std::move(completion));
}

} // namespace consolidator::vst3::features::tracks::controllers
