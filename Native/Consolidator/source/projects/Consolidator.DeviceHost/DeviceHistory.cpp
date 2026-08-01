#include "DeviceHistory.h"

#include <iterator>
#include <utility>

namespace consolidator::host {

bool DeviceHistory::Begin(DeviceHistoryState state, domain::StoreRevision revision, std::string operationId, std::string linkId) {
    if (transactionBefore) return false;
    transactionBefore = Entry{ std::move(state), std::move(operationId), std::move(linkId) };
    transactionRevision = revision;
    return true;
}

bool DeviceHistory::End(domain::StoreRevision revision, const std::string& operationId) {
    if (!transactionBefore || transactionBefore->operationId != operationId) return false;
    const auto changed = revision != transactionRevision;
    if (changed) Push(undoStates, std::move(*transactionBefore));
    transactionBefore.reset();
    if (changed) redoStates.clear();
    return changed;
}

void DeviceHistory::Record(DeviceHistoryState state, std::string operationId, std::string linkId) {
    Push(undoStates, { std::move(state), std::move(operationId), std::move(linkId) });
    redoStates.clear();
}

std::optional<HistoryRestore> DeviceHistory::Undo(DeviceHistoryState current) {
    if (undoStates.empty()) return std::nullopt;
    auto entry = std::move(undoStates.back());
    undoStates.pop_back();
    Push(redoStates, { std::move(current), entry.operationId, entry.linkId });
    return HistoryRestore{ std::move(entry.state), std::move(entry.operationId), std::move(entry.linkId) };
}

std::optional<HistoryRestore> DeviceHistory::Redo(DeviceHistoryState current) {
    if (redoStates.empty()) return std::nullopt;
    auto entry = std::move(redoStates.back());
    redoStates.pop_back();
    Push(undoStates, { std::move(current), entry.operationId, entry.linkId });
    return HistoryRestore{ std::move(entry.state), std::move(entry.operationId), std::move(entry.linkId) };
}

std::optional<HistoryRestore> DeviceHistory::Restore(
    const std::string& operationId,
    bool isUndo,
    DeviceHistoryState current
) {
    auto& source = isUndo ? undoStates : redoStates;
    auto& destination = isUndo ? redoStates : undoStates;
    for (auto iterator = source.rbegin(); iterator != source.rend(); ++iterator) {
        if (iterator->operationId != operationId) continue;
        auto entry = std::move(*iterator);
        source.erase(std::next(iterator).base());
        Push(destination, { std::move(current), entry.operationId, entry.linkId });
        return HistoryRestore{ std::move(entry.state), std::move(entry.operationId), std::move(entry.linkId) };
    }
    return std::nullopt;
}

void DeviceHistory::Clear() {
    undoStates.clear();
    redoStates.clear();
    transactionBefore.reset();
}

void DeviceHistory::Push(std::vector<Entry>& states, Entry state) {
    if (states.size() == capacity) states.erase(states.begin());
    states.push_back(std::move(state));
}

} // namespace consolidator::host
