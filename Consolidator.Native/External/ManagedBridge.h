#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "ManagedInterop.h"

namespace consolidator::max
{

class ManagedBridge
{
public:
    ManagedBridge();
    ~ManagedBridge();

    ManagedBridge(const ManagedBridge&) = delete;
    ManagedBridge& operator=(const ManagedBridge&) = delete;

    [[nodiscard]] bool IsLoaded() const noexcept;

    [[nodiscard]] InstanceId RegisterInstance(
        void* context,
        ManagedOutputCallback outputCallback,
        SharedDspExchange* dspExchange) const;

    void UnregisterInstance(InstanceId instanceId) const;

    void SendManagedMessage(
        InstanceId instanceId,
        const char* selector,
        const NativeAtom* atoms,
        std::size_t atomCount) const;

    void Prepare(
        InstanceId instanceId,
        double sampleRate,
        std::size_t maximumFrameCount) const;

    void SendAudio(
        InstanceId instanceId,
        const double* mainLeft,
        const double* mainRight,
        const double* referenceLeft,
        const double* referenceRight,
        std::size_t frameCount) const;

private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};

} // namespace consolidator::max