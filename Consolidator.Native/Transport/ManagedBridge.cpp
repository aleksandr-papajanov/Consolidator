#include "ManagedBridge.h"

#include <memory>
#include <mutex>

#include <Windows.h>
#undef SendMessage

namespace consolidator::transport
{

namespace
{

HMODULE LoadManagedLibrary(const std::wstring& libraryPath, ManagedLogCallback logCallback)
{
    const auto report = [logCallback](const char* message)
    {
        if (logCallback != nullptr)
        {
            logCallback(nullptr, message);
        }
    };
    const auto separator = libraryPath.find_last_of(L"\\/");
    if (libraryPath.empty() || separator == std::wstring::npos)
    {
        report("Managed library requires an absolute host-resolved path.");
        return nullptr;
    }

    const auto nloptPath = libraryPath.substr(0, separator + 1) + L"nlopt.dll";
    if (LoadLibraryExW(nloptPath.c_str(), nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS) == nullptr)
    {
        report("Failed to load NLopt beside the Managed library.");
        return nullptr;
    }

    const auto library = LoadLibraryExW(libraryPath.c_str(), nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (library == nullptr)
    {
        report("Failed to load the Managed library.");
    }
    return library;
}

class ManagedRuntime
{
public:
    ManagedRuntime() = default;

    ManagedRuntime(const ManagedRuntime&) = delete;
    ManagedRuntime& operator=(const ManagedRuntime&) = delete;

    [[nodiscard]] bool IsLoaded() const noexcept;
    bool Load(const std::wstring& libraryPath, ManagedLogCallback logCallback);
    void Activate(ManagedLogCallback logCallback) const noexcept;
    void Deactivate() const noexcept;

private:
    friend class ManagedBridge;

    using RegisterInstanceFn = InstanceId (__cdecl *)(
        void*,
        ManagedOutputCallback,
        DspStateExchange*,
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

bool ManagedRuntime::Load(const std::wstring& libraryPath, ManagedLogCallback logCallback)
{
    if (IsLoaded())
    {
        return true;
    }

    if (library)
    {
        return false;
    }

    library = LoadManagedLibrary(libraryPath, logCallback);

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

    if (!IsLoaded() && logCallback != nullptr)
    {
        logCallback(nullptr, "Managed library is missing required Consolidator C ABI exports.");
    }
    return IsLoaded();
}

void ManagedRuntime::Activate(ManagedLogCallback logCallback) const noexcept
{
    if (IsLoaded())
    {
        setLogCallback(nullptr, logCallback);
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
std::uint32_t activeBridgeCount{};

}

struct ManagedBridge::Implementation
{
    ManagedRuntime* runtime{};
    bool active{};
};

ManagedBridge::ManagedBridge(const std::wstring& libraryPath, ManagedLogCallback logCallback)
    : implementation_(new Implementation{})
{
    const std::lock_guard lock(runtimeMutex);
    implementation_->runtime = &GetManagedRuntime();
    if (!implementation_->runtime->Load(libraryPath, logCallback))
    {
        return;
    }
    if (activeBridgeCount == 0)
    {
        implementation_->runtime->Activate(logCallback);
    }

    ++activeBridgeCount;
    implementation_->active = true;
}

ManagedBridge::~ManagedBridge()
{
    const std::lock_guard lock(runtimeMutex);
    if (!implementation_->active)
    {
        return;
    }
    --activeBridgeCount;
    if (activeBridgeCount == 0)
    {
        implementation_->runtime->Deactivate();
    }
}

bool ManagedBridge::IsLoaded() const noexcept
{
    return implementation_->active && implementation_->runtime->IsLoaded();
}

InstanceId ManagedBridge::RegisterInstance(
    void* context,
    ManagedOutputCallback outputCallback,
    DspStateExchange* dspExchange,
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

} // namespace consolidator::transport
