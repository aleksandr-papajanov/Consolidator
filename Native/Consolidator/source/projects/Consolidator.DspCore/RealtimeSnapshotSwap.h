#pragma once

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace consolidator::dspcore {

template <typename Snapshot>
class RealtimeSnapshotSwap final {
public:
    RealtimeSnapshotSwap()
        : current(new Snapshot{}) {}

    ~RealtimeSnapshotSwap() {
        delete current.load();
    }

    RealtimeSnapshotSwap(const RealtimeSnapshotSwap&) = delete;
    RealtimeSnapshotSwap& operator=(const RealtimeSnapshotSwap&) = delete;

    template <typename Reader>
    std::invoke_result_t<Reader, Snapshot&> Read(Reader&& reader) {
        Snapshot* snapshot = nullptr;
        do {
            snapshot = current.load(std::memory_order_acquire);
            hazard.store(snapshot, std::memory_order_release);
        } while (snapshot != current.load(std::memory_order_acquire));

        struct HazardGuard final {
            std::atomic<Snapshot*>& hazard;
            ~HazardGuard() { hazard.store(nullptr, std::memory_order_release); }
        } guard{ hazard };
        return std::invoke(std::forward<Reader>(reader), *snapshot);
    }

    template <typename Writer>
    std::invoke_result_t<Writer, Snapshot&> UpdateCurrent(Writer&& writer) {
        return std::invoke(std::forward<Writer>(writer),
            *current.load(std::memory_order_acquire));
    }

    void Replace(std::unique_ptr<Snapshot> replacement) {
        if (!replacement) throw std::invalid_argument("Replacement snapshot must not be null");
        auto* previous = current.exchange(replacement.release(), std::memory_order_acq_rel);
        if (previous) retired.emplace_back(previous);
        Reclaim();
    }

private:
    void Reclaim() {
        const auto* active = hazard.load(std::memory_order_acquire);
        retired.erase(std::remove_if(retired.begin(), retired.end(),
            [active](const auto& snapshot) { return snapshot.get() != active; }), retired.end());
    }

    std::atomic<Snapshot*> current;
    std::atomic<Snapshot*> hazard = nullptr;
    std::vector<std::unique_ptr<Snapshot>> retired;
};

} // namespace consolidator::dspcore
