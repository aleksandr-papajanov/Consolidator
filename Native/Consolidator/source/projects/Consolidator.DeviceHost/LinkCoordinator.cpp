#include "LinkCoordinator.h"

#include "DeviceHost.h"
#include "Definitions/Definitions.h"
#include "Models/ParameterDefinitions.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace consolidator::host {

LinkCoordinator& LinkCoordinator::Instance() {
    static LinkCoordinator coordinator;
    return coordinator;
}

void LinkCoordinator::Upsert(LinkCoordinatorEntry entry) {
    if (entry.runtimeId.empty()) return;
    std::lock_guard lock(mutex);
    RemoveLinksFor(entry.runtimeId);
    auto runtimeId = entry.runtimeId;
    auto eq = entry.eq;
    entries.insert_or_assign(runtimeId, std::move(entry));
    AddLinksFor(runtimeId, eq);
}

void LinkCoordinator::AddLinksFor(const std::string& runtimeId, const models::EqSnapshot& eq) {
    for (const auto& bank : eq.banks) {
        if (bank.linkId.empty()) continue;
        linkMembers[bank.linkId].push_back(runtimeId);
    }
}

void LinkCoordinator::RemoveLinksFor(const std::string& runtimeId) {
    for (auto& [linkId, members] : linkMembers) {
        members.erase(std::remove(members.begin(), members.end(), runtimeId), members.end());
    }
}

void LinkCoordinator::RegisterHost(const std::string& runtimeId, DeviceHost& host) {
    if (runtimeId.empty()) return;
    std::lock_guard lock(mutex);
    hosts.insert_or_assign(runtimeId, &host);
}

void LinkCoordinator::Remove(const std::string& runtimeId) {
    if (runtimeId.empty()) return;
    std::lock_guard lock(mutex);
    RemoveLinksFor(runtimeId);
    entries.erase(runtimeId);
    hosts.erase(runtimeId);
}

void LinkCoordinator::Dispatch(const LinkedFilterGesture& gesture) {
    struct Recipient {
        DeviceHost* host;
        std::string runtimeId;
        long bankId;
        double currentValue;
    };
    std::vector<Recipient> recipients;
    {
        std::lock_guard lock(mutex);
        const auto members = linkMembers.find(gesture.linkId);
        if (members == linkMembers.end()) return;
        for (const auto& runtimeId : members->second) {
            if (runtimeId == gesture.sourceRuntimeId) continue;
            auto entry = entries.find(runtimeId);
            if (entry == entries.end()) continue;
            const auto hostIt = hosts.find(runtimeId);
            if (hostIt == hosts.end()) continue;
            const auto& banks = entry->second.eq.banks;
            const auto bank = std::find_if(banks.begin(), banks.end(),
                [&gesture](const auto& b) { return b.linkId == gesture.linkId; });
            if (bank == banks.end()) continue;
            const auto* filter = bank->FindFilter(gesture.filterId);
            if (!filter || gesture.parameterIndex >= filter->values.size()) continue;
            recipients.push_back({ hostIt->second, runtimeId, bank->bankId, filter->values[gesture.parameterIndex] });
        }
    }
    for (const auto& recipient : recipients) {
        const auto definition = domain::FilterDefinitions().find(gesture.filterId);
        if (definition == domain::FilterDefinitions().end() ||
            gesture.parameterIndex >= definition->second.parameters.size()) continue;
        const auto& range = definition->second.parameters[gesture.parameterIndex].range;
        const auto value = range.Denormalize(range.Normalize(recipient.currentValue) +
            gesture.targetNormalized - gesture.sourceNormalized);
        if (!std::isfinite(value)) continue;
        recipient.host->Handle(domain::SetEqParameterIndexCommand{
            {}, { recipient.bankId }, { gesture.filterId }, gesture.parameterIndex, value
        });
        // Update coordinator cache so next gesture reads correct source
        {
            std::lock_guard lock(mutex);
            auto entry = entries.find(recipient.runtimeId);
            if (entry == entries.end()) continue;
            auto* bank = entry->second.eq.FindBank(recipient.bankId);
            if (!bank) continue;
            auto* filter = bank->FindFilter(gesture.filterId);
            if (!filter || gesture.parameterIndex >= filter->values.size()) continue;
            filter->values[gesture.parameterIndex] = value;
        }
    }
}

void LinkCoordinator::Dispatch(const LinkedProcessorGesture& gesture) {
    struct Recipient {
        DeviceHost* host;
        double currentValue;
    };
    std::vector<Recipient> recipients;
    {
        std::lock_guard lock(mutex);
        const auto members = linkMembers.find(gesture.linkId);
        if (members == linkMembers.end()) return;
        for (const auto& runtimeId : members->second) {
            if (runtimeId == gesture.sourceRuntimeId) continue;
            const auto entry = entries.find(runtimeId);
            const auto hostIt = hosts.find(runtimeId);
            if (entry == entries.end() || hostIt == hosts.end()) continue;
            const auto& banks = entry->second.eq.banks;
            const auto bank = std::find_if(banks.begin(), banks.end(),
                [&gesture](const auto& b) { return b.linkId == gesture.linkId; });
            if (bank == banks.end()) continue;
            const auto current = ReadProcessorValue(entry->second.processor, gesture.device, gesture.parameter);
            if (!current) continue;
            recipients.push_back({ hostIt->second, *current });
        }
    }
    for (const auto& recipient : recipients) {
        const auto key = gesture.device + "." + gesture.parameter;
        const auto definition = models::ParameterDefinitions().find(key);
        if (definition == models::ParameterDefinitions().end()) continue;
        const auto value = definition->second.Denormalize(definition->second.Normalize(recipient.currentValue) +
            gesture.targetNormalized - gesture.sourceNormalized);
        if (!std::isfinite(value)) continue;
        domain::Command command;
        if (gesture.device == "input_gain" || gesture.device == "output_gain") {
            command = domain::SetGainParameterCommand{
                {}, gesture.device == "input_gain"
                    ? domain::GainStage::Input : domain::GainStage::Output, value
            };
        } else if (gesture.device == "compressor") {
            command = domain::SetCompressorParameterCommand{ {}, gesture.parameter, value };
        } else if (gesture.device == "saturator") {
            command = domain::SetSaturatorParameterCommand{ {}, gesture.parameter, value };
        } else continue;
        recipient.host->Handle(command);
    }
}

void LinkCoordinator::DispatchCommandToHost(const std::string& runtimeId, const domain::Command& command) const {
    DeviceHost* target = nullptr;
    {
        std::lock_guard lock(mutex);
        const auto it = hosts.find(runtimeId);
        if (it != hosts.end()) target = it->second;
    }
    if (target) target->Handle(command);
}

void LinkCoordinator::DispatchCommandToGroup(const std::string& linkId, const domain::Command& command) const {
    if (linkId.empty()) return;
    std::vector<DeviceHost*> recipients;
    {
        std::lock_guard lock(mutex);
        const auto members = linkMembers.find(linkId);
        if (members == linkMembers.end()) return;
        for (const auto& runtimeId : members->second) {
            const auto host = hosts.find(runtimeId);
            if (host != hosts.end()) recipients.push_back(host->second);
        }
    }
    for (auto* host : recipients) host->Handle(command);
}

void LinkCoordinator::DispatchCommandToAll(const domain::Command& command) const {
    std::vector<DeviceHost*> recipients;
    {
        std::lock_guard lock(mutex);
        for (const auto& [runtimeId, host] : hosts) {
            recipients.push_back(host);
        }
    }
    for (auto* host : recipients) host->Handle(command);
}

void LinkCoordinator::UpdateState(
    const std::string& runtimeId,
    domain::StoreRevision revision,
    const models::EqSnapshot& eq,
    const models::ProcessorState& processor
) {
    std::lock_guard lock(mutex);
    const auto entry = entries.find(runtimeId);
    if (entry == entries.end()) return;
    RemoveLinksFor(runtimeId);
    entry->second.revision = revision;
    entry->second.eq = eq;
    entry->second.processor = processor;
    AddLinksFor(runtimeId, eq);
}

std::vector<LinkCoordinatorEntry> LinkCoordinator::Entries() const {
    std::lock_guard lock(mutex);
    std::vector<LinkCoordinatorEntry> result;
    result.reserve(entries.size());
    for (const auto& [runtimeId, entry] : entries) {
        result.push_back(entry);
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        if (left.trackOrder != right.trackOrder) return left.trackOrder < right.trackOrder;
        return left.runtimeId < right.runtimeId;
    });
    return result;
}

std::optional<LinkCoordinatorEntry> LinkCoordinator::Find(
    const std::string& runtimeId
) const {
    std::lock_guard lock(mutex);
    const auto entry = entries.find(runtimeId);
    if (entry == entries.end()) return std::nullopt;
    return entry->second;
}

std::vector<LinkCoordinatorMember> LinkCoordinator::Members(const std::string& linkId) const {
    if (linkId.empty()) return {};
    std::lock_guard lock(mutex);
    std::vector<LinkCoordinatorMember> result;
    for (const auto& [runtimeId, entry] : entries) {
        for (const auto& bank : entry.eq.banks) {
            if (bank.linkId == linkId) result.push_back({ runtimeId, bank.bankId });
        }
    }
    return result;
}

std::optional<double> LinkCoordinator::ReadProcessorValue(
    const models::ProcessorState& processor,
    const std::string& device,
    const std::string& parameter
) {
    if (device == "input_gain" && parameter == "gain") return processor.inputGain.gainDb;
    if (device == "output_gain" && parameter == "gain") return processor.outputGain.gainDb;
    if (device == "compressor") {
        const auto& state = processor.compressor;
        if (parameter == "attack") return state.attackMs;
        if (parameter == "release") return state.releaseMs;
        if (parameter == "threshold") return state.thresholdDb;
        if (parameter == "output") return state.outputDb;
        if (parameter == "mix") return state.mix;
    }
    if (device == "saturator") {
        const auto& state = processor.saturator;
        if (parameter == "saturation") return state.saturation;
        if (parameter == "output") return state.outputDb;
    }
    return std::nullopt;
}

} // namespace consolidator::host