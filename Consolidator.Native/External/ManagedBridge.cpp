#include "ManagedBridge.h"

#include <Windows.h>
#undef SendMessage

#include <cstring>
#include <mutex>
#include <stdexcept>

#include "c74_min_api.h"

namespace consolidator::max
{

namespace
{

constexpr auto kManagedLibraryName = "Consolidator.Managed.dll";

std::mutex g_logCallbackMutex;
std::size_t g_logCallbackUsers{};

void __cdecl ManagedLogCallbackHandler(void*, const char* message)
{
    if (message != nullptr)
    {
        c74::max::post(
            "[Consolidator.Managed] %s",
            message);
    }
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

}

struct ManagedBridge::Implementation
{
    using RegisterInstanceFn = InstanceId (__cdecl *)(
        void*,
        ManagedOutputCallback);

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
            InstanceId,
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

ManagedBridge::ManagedBridge(c74::min::logger& errorLogger)
    : implementation_(new Implementation{})
    , errorLogger_(errorLogger)
{
    auto& implementation = *implementation_;

    implementation.library = LoadManagedLibrary();

    if (!implementation.library)
    {
        const auto error = GetLastError();

        errorLogger_
            << "Consolidator: LoadLibrary failed. Error: "
            << error
            << c74::min::endl;

        return;
    }

    implementation.registerInstance =
        reinterpret_cast<Implementation::RegisterInstanceFn>(
            GetProcAddress(
                implementation.library,
                "ConsolidatorRegisterInstance"));

    implementation.unregisterInstance =
        reinterpret_cast<Implementation::UnregisterInstanceFn>(
            GetProcAddress(
                implementation.library,
                "ConsolidatorUnregisterInstance"));

    implementation.setLogCallback =
        reinterpret_cast<Implementation::SetLogCallbackFn>(
            GetProcAddress(
                implementation.library,
                "ConsolidatorSetLogCallback"));

    implementation.sendMessage =
        reinterpret_cast<Implementation::SendMessageFn>(
            GetProcAddress(
                implementation.library,
                "ConsolidatorSendMessage"));

    implementation.prepare =
        reinterpret_cast<Implementation::PrepareFn>(
            GetProcAddress(
                implementation.library,
                "ConsolidatorPrepare"));

    implementation.sendAudio =
        reinterpret_cast<Implementation::SendAudioFn>(
            GetProcAddress(
                implementation.library,
                "ConsolidatorSendAudio"));
}

ManagedBridge::~ManagedBridge()
{
    if (implementation_->library && implementation_->setLogCallback)
    {
        std::lock_guard lock{ g_logCallbackMutex };

        if (g_logCallbackUsers > 0)
        {
            --g_logCallbackUsers;
        }

        if (g_logCallbackUsers == 0)
        {
            implementation_->setLogCallback(nullptr, nullptr);
        }
    }

    if (implementation_->library)
    {
        FreeLibrary(implementation_->library);
    }

    delete implementation_;
}

bool ManagedBridge::IsLoaded() const noexcept
{
    const auto& implementation = *implementation_;

    return implementation.library &&
           implementation.registerInstance &&
           implementation.unregisterInstance &&
           implementation.setLogCallback &&
           implementation.sendMessage &&
           implementation.prepare &&
           implementation.sendAudio;
}

InstanceId ManagedBridge::RegisterInstance(
    void* context,
    ManagedOutputCallback outputCallback) const
{
    {
        std::lock_guard lock{ g_logCallbackMutex };

        if (g_logCallbackUsers == 0)
        {
            implementation_->setLogCallback(
                nullptr,
                ManagedLogCallbackHandler);
        }

        ++g_logCallbackUsers;
    }

    return implementation_->registerInstance(
        context,
        outputCallback);
}

void ManagedBridge::UnregisterInstance(InstanceId instanceId) const
{
    implementation_->unregisterInstance(instanceId);
}

void ManagedBridge::SendManagedMessage(
    InstanceId instanceId,
    const char* selector,
    const NativeAtom* atoms,
    std::size_t atomCount) const
{
    implementation_->sendMessage(
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
    implementation_->prepare(
        instanceId,
        sampleRate,
        maximumFrameCount);
}

void ManagedBridge::SendAudio(
    InstanceId instanceId,
    const double* mainLeft,
    const double* mainRight,
    const double* referenceLeft,
    const double* referenceRight,
    std::size_t frameCount) const
{
    implementation_->sendAudio(
        instanceId,
        mainLeft,
        mainRight,
        referenceLeft,
        referenceRight,
        frameCount);
}

} // namespace consolidator::max