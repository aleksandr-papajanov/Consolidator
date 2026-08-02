#include "LinkCoordinator.h"

#include <algorithm>
#include <utility>

namespace consolidator::host {

LinkCoordinator& LinkCoordinator::Instance() {
    static LinkCoordinator coordinator;
    return coordinator;
}

void LinkCoordinator::Upsert(LinkCoordinatorEntry entry) {
    if (entry.runtimeId.empty()) return;
    std::lock_guard lock(mutex);
    entries.insert_or_assign(entry.runtimeId, std::move(entry));
}

void LinkCoordinator::RegisterCallbacks(
    const std::string& runtimeId,
    LinkCoordinatorCallbacks value
) {
    if (runtimeId.empty()) return;
    std::lock_guard lock(mutex);
    callbacks.insert_or_assign(runtimeId, std::move(value));
}

void LinkCoordinator::Remove(const std::string& runtimeId) {
    if (runtimeId.empty()) return;
    std::lock_guard lock(mutex);
    entries.erase(runtimeId);
    callbacks.erase(runtimeId);
}

void LinkCoordinator::Dispatch(const LinkedFilterGesture& gesture) const {
    std::vector<std::function<void(const LinkedFilterGesture&)>> recipients;
    {
        std::lock_guard lock(mutex);
        for (const auto& [runtimeId, entry] : entries) {
            const auto linked = std::any_of(entry.eq.banks.begin(), entry.eq.banks.end(),
                [&gesture](const auto& bank) { return bank.linkId == gesture.linkId; });
            const auto callback = callbacks.find(runtimeId);
            if (linked && callback != callbacks.end() && callback->second.applyFilter) {
                recipients.push_back(callback->second.applyFilter);
            }
        }
    }
    for (const auto& recipient : recipients) recipient(gesture);
}

void LinkCoordinator::Dispatch(const LinkedProcessorGesture& gesture) const {
    std::vector<std::function<void(const LinkedProcessorGesture&)>> recipients;
    {
        std::lock_guard lock(mutex);
        for (const auto& [runtimeId, entry] : entries) {
            const auto linked = std::any_of(entry.eq.banks.begin(), entry.eq.banks.end(),
                [&gesture](const auto& bank) { return bank.linkId == gesture.linkId; });
            const auto callback = callbacks.find(runtimeId);
            if (linked && callback != callbacks.end() && callback->second.applyProcessor) {
                recipients.push_back(callback->second.applyProcessor);
            }
        }
    }
    for (const auto& recipient : recipients) recipient(gesture);
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
    entry->second.revision = revision;
    entry->second.eq = eq;
    entry->second.processor = processor;
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

} // namespace consolidator::host
