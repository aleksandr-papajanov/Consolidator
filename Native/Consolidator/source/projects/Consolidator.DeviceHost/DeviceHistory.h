#pragma once

#include "States/States.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace consolidator::host {

struct DeviceHistoryState {
    domain::EqState eq;
    domain::ProcessorState processor;
};

struct HistoryRestore {
    DeviceHistoryState state;
    std::string operationId;
    std::string linkId;
};

class DeviceHistory final {
public:
    explicit DeviceHistory(std::size_t capacity = 128) : capacity(capacity) {}

    bool Begin(DeviceHistoryState state, domain::StoreRevision revision, std::string operationId, std::string linkId);
    bool End(domain::StoreRevision revision, const std::string& operationId);
    void Record(DeviceHistoryState state, std::string operationId = {}, std::string linkId = {});
    std::optional<HistoryRestore> Undo(DeviceHistoryState current);
    std::optional<HistoryRestore> Redo(DeviceHistoryState current);
    std::optional<HistoryRestore> Restore(const std::string& operationId, bool isUndo, DeviceHistoryState current);
    void Clear();

    bool InTransaction() const noexcept { return transactionBefore.has_value(); }
    bool IsTransaction(std::string_view operationId) const noexcept {
        return transactionBefore && transactionBefore->operationId == operationId;
    }
    bool CanUndo() const noexcept { return !undoStates.empty(); }
    bool CanRedo() const noexcept { return !redoStates.empty(); }

private:
    struct Entry {
        DeviceHistoryState state;
        std::string operationId;
        std::string linkId;
    };

    void Push(std::vector<Entry>& states, Entry state);

    std::size_t capacity;
    std::vector<Entry> undoStates;
    std::vector<Entry> redoStates;
    std::optional<Entry> transactionBefore;
    domain::StoreRevision transactionRevision{};
};

} // namespace consolidator::host
