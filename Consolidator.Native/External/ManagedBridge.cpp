#include "ManagedBridge.h"

#include <Windows.h>
#undef SendMessage

#include <cstring>
#include <memory>
#include <mutex>

#include "c74_min_api.h"

namespace consolidator::max
{

namespace
{

constexpr auto kManagedLibraryName = "Consolidator.Managed.dll";

void __cdecl ManagedLogCallbackHandler(
    void*,
    const char* message) noexcept
{
    if (message == nullptr)
    {
        return;
    }

    c74::max::post(
        "[Consolidator.Managed] %s",
        message);
}

HMODULE LoadManagedLibrary()
{
    char filename[c74::max::MAX_FILENAME_CHARS]{};
    std::strncpy(filename, kManagedLibraryName, sizeof(filename) - 1);

    short path = 0;
    c74::max::t_fourcc type = 0;
    if (c74::max::locatefile_extended(
            filename,
            &path,
            &type,
            nullptr,
            0) != 0)
    {
        return nullptr;
    }

    char absolutePath[c74::max::MAX_PATH_CHARS]{};
    if (c74::max::path_toabsolutesystempath(
            path,
            filename,
            absolutePath) != c74::max::MAX_ERR_NONE)
    {
        return nullptr;
    }

    return LoadLibraryA(absolutePath);
}

class ManagedRuntime
{
public:
    ManagedRuntime() = default;

    ManagedRuntime(const ManagedRuntime&) = delete;
    ManagedRuntime& operator=(const ManagedRuntime&) = delete;

    [[nodiscard]] bool IsLoaded() const noexcept;
    bool Load() noexcept;
    void Activate() const noexcept;
    void Deactivate() const noexcept;

private:
    friend class ManagedBridge;

    using RegisterInstanceFn = InstanceId (__cdecl *)(
        void*,
        ManagedOutputCallback,
        SharedDspExchange*,
        AudioInputHandle*);

    using UnregisterInstanceFn = void (__cdecl *)(InstanceId);

    using SetLogCallbackFn =
        void (__cdecl *)(
            void*,
            ManagedLogCallback);

    using SendMessageFn =
        void (__cdecl *)(
            InstanceId,
            const char*,
            const NativeAtom*,
            std::size_t);

    using PrepareFn =
        void (__cdecl *)(
            InstanceId,
            double,
            std::size_t);

    using SendAudioFn =
        void (__cdecl *)(
            AudioInputHandle,
            const double*,
            const double*,
            const double*,
            const double*,
            std::size_t);

    using ShutdownFn = void (__cdecl *)();

    ManagedCapturePersistence capturePersistence{};
    ManagedFreePersistence freePersistence{};
    ManagedRestorePersistence restorePersistence{};

    HMODULE library{};
    RegisterInstanceFn registerInstance{};
    UnregisterInstanceFn unregisterInstance{};
    SetLogCallbackFn setLogCallback{};
    SendMessageFn sendMessage{};
    PrepareFn prepare{};
    SendAudioFn sendAudio{};
    ShutdownFn shutdown{};
};

bool ManagedRuntime::Load() noexcept
{
    if (IsLoaded())
    {
        return true;
    }

    if (library)
    {
        return false;
    }

    library = LoadManagedLibrary();

    if (!library)
    {
        return false;
    }

    registerInstance = reinterpret_cast<RegisterInstanceFn>(
        GetProcAddress(library, "ConsolidatorRegisterInstance"));
    unregisterInstance = reinterpret_cast<UnregisterInstanceFn>(
        GetProcAddress(library, "ConsolidatorUnregisterInstance"));
    setLogCallback = reinterpret_cast<SetLogCallbackFn>(
        GetProcAddress(library, "ConsolidatorSetLogCallback"));
    sendMessage = reinterpret_cast<SendMessageFn>(
        GetProcAddress(library, "ConsolidatorSendMessage"));
    prepare = reinterpret_cast<PrepareFn>(
        GetProcAddress(library, "ConsolidatorPrepare"));
    sendAudio = reinterpret_cast<SendAudioFn>(
        GetProcAddress(library, "ConsolidatorSendAudio"));
    shutdown = reinterpret_cast<ShutdownFn>(
        GetProcAddress(library, "ConsolidatorShutdown"));
    capturePersistence = reinterpret_cast<ManagedCapturePersistence>(
        GetProcAddress(library, "ConsolidatorCapturePersistence"));
    freePersistence = reinterpret_cast<ManagedFreePersistence>(
        GetProcAddress(library, "ConsolidatorFreePersistence"));
    restorePersistence = reinterpret_cast<ManagedRestorePersistence>(
        GetProcAddress(library, "ConsolidatorRestorePersistence"));

    return IsLoaded();
}

void ManagedRuntime::Activate() const noexcept
{
    if (IsLoaded())
    {
        setLogCallback(nullptr, ManagedLogCallbackHandler);
    }
}

void ManagedRuntime::Deactivate() const noexcept
{
    if (!IsLoaded())
    {
        return;
    }

    shutdown();
    setLogCallback(nullptr, nullptr);
}

bool ManagedRuntime::IsLoaded() const noexcept
{
    return library &&
           registerInstance &&
           unregisterInstance &&
           setLogCallback &&
           sendMessage &&
           prepare &&
           sendAudio &&
           shutdown &&
           capturePersistence &&
           freePersistence &&
           restorePersistence;
}

ManagedRuntime& GetManagedRuntime()
{
    static ManagedRuntime runtime;
    return runtime;
}

std::mutex runtimeMutex;
std::uint32_t activeExternalCount{};

}

struct ManagedBridge::Implementation
{
    ManagedRuntime* runtime{};
};

ManagedBridge::ManagedBridge()
    : implementation_(new Implementation{})
{
    const std::lock_guard lock(runtimeMutex);
    implementation_->runtime = &GetManagedRuntime();
    implementation_->runtime->Load();
    if (activeExternalCount == 0)
    {
        implementation_->runtime->Activate();
    }

    ++activeExternalCount;
}

ManagedBridge::~ManagedBridge()
{
    const std::lock_guard lock(runtimeMutex);
    --activeExternalCount;
    if (activeExternalCount == 0)
    {
        implementation_->runtime->Deactivate();
    }
}

bool ManagedBridge::IsLoaded() const noexcept
{
    return implementation_->runtime->IsLoaded();
}

InstanceId ManagedBridge::RegisterInstance(
    void* context,
    ManagedOutputCallback outputCallback,
    SharedDspExchange* dspExchange,
    AudioInputHandle* audioInputHandle) const
{
    const auto* runtime = implementation_->runtime;

    if (!runtime->IsLoaded())
    {
        return 0;
    }

    return runtime->registerInstance(
        context,
        outputCallback,
        dspExchange,
        audioInputHandle);
}

void ManagedBridge::UnregisterInstance(InstanceId instanceId) const
{
    const auto* runtime = implementation_->runtime;

    if (!runtime->IsLoaded())
    {
        return;
    }

    runtime->unregisterInstance(instanceId);
}

void ManagedBridge::SendManagedMessage(
    InstanceId instanceId,
    const char* selector,
    const NativeAtom* atoms,
    std::size_t atomCount) const
{
    const auto* runtime = implementation_->runtime;

    if (!runtime->IsLoaded())
    {
        return;
    }

    runtime->sendMessage(
        instanceId,
        selector,
        atoms,
        atomCount);
}

void ManagedBridge::Prepare(
    InstanceId instanceId,
    double sampleRate,
    std::size_t maximumFrameCount) const
{
    const auto* runtime = implementation_->runtime;

    if (!runtime->IsLoaded())
    {
        return;
    }

    runtime->prepare(
        instanceId,
        sampleRate,
        maximumFrameCount);
}

void ManagedBridge::SendAudio(
    AudioInputHandle audioInputHandle,
    const double* mainLeft,
    const double* mainRight,
    const double* referenceLeft,
    const double* referenceRight,
    std::size_t frameCount) const
{
    const auto* runtime = implementation_->runtime;

    if (!runtime->IsLoaded())
    {
        return;
    }

    runtime->sendAudio(
        audioInputHandle,
        mainLeft,
        mainRight,
        referenceLeft,
        referenceRight,
        frameCount);
}

std::string ManagedBridge::CapturePersistence(InstanceId instanceId) const
{
    const auto* runtime = implementation_->runtime;
    if (!runtime->IsLoaded())
    {
        return {};
    }

    char* data = nullptr;
    std::size_t length = 0;
    if (runtime->capturePersistence(instanceId, &data, &length) == 0)
    {
        return {};
    }

    if (data == nullptr || length == 0)
    {
        if (data != nullptr)
        {
            runtime->freePersistence(data);
        }
        return {};
    }

    const auto deleter = [freePersistence = runtime->freePersistence](char* pointer)
    {
        if (pointer != nullptr)
        {
            freePersistence(pointer);
        }
    };
    std::unique_ptr<char, decltype(deleter)> owner(data, deleter);
    auto result = std::string(owner.get(), length);
    return result;
}

bool ManagedBridge::RestorePersistence(
    InstanceId instanceId,
    const char* data,
    std::size_t length) const
{
    const auto* runtime = implementation_->runtime;
    const auto result = runtime->IsLoaded() &&
        data != nullptr &&
        runtime->restorePersistence(instanceId, data, length) != 0;
    return result;
}

} // namespace consolidator::max
