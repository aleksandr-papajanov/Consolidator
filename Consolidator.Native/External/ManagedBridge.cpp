#include "ManagedBridge.h"

#include <Windows.h>
#undef SendMessage

#include <cstring>

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
    ManagedRuntime();
    ~ManagedRuntime();

    ManagedRuntime(const ManagedRuntime&) = delete;
    ManagedRuntime& operator=(const ManagedRuntime&) = delete;

    [[nodiscard]] bool IsLoaded() const noexcept;

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

    HMODULE library{};
    RegisterInstanceFn registerInstance{};
    UnregisterInstanceFn unregisterInstance{};
    SetLogCallbackFn setLogCallback{};
    SendMessageFn sendMessage{};
    PrepareFn prepare{};
    SendAudioFn sendAudio{};
};

ManagedRuntime::ManagedRuntime()
{
    library = LoadManagedLibrary();

    if (!library)
    {
        return;
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

    if (IsLoaded())
    {
        setLogCallback(nullptr, ManagedLogCallbackHandler);
    }
}

ManagedRuntime::~ManagedRuntime()
{
    if (!library)
    {
        return;
    }

    if (setLogCallback)
    {
        setLogCallback(nullptr, nullptr);
    }

    FreeLibrary(library);
}

bool ManagedRuntime::IsLoaded() const noexcept
{
    return library &&
           registerInstance &&
           unregisterInstance &&
           setLogCallback &&
           sendMessage &&
           prepare &&
           sendAudio;
}

ManagedRuntime& GetManagedRuntime()
{
    static ManagedRuntime runtime;
    return runtime;
}

}

struct ManagedBridge::Implementation
{
    ManagedRuntime* runtime{};
};

ManagedBridge::ManagedBridge()
    : implementation_(new Implementation{})
{
    implementation_->runtime = &GetManagedRuntime();
}

ManagedBridge::~ManagedBridge() = default;

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

} // namespace consolidator::max