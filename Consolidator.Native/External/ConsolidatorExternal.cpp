#include "ConsolidatorExternal.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

#ifdef SendMessage
#undef SendMessage
#endif

namespace consolidator::max
{

using namespace c74::min;

namespace
{

constexpr std::size_t kMaxControlFramesPerDrain = 32;
constexpr std::size_t kMaxControlAtomsPerDrain = 4096;

}

ConsolidatorExternal::ConsolidatorExternal()
    : managed_()
{
    value.set_visibility(c74::min::visibility::hide);

    if (maxobj() != nullptr)
    {
        const auto error = c74::max::object_parameter_init_flags(
            maxobj(),
            c74::max::PARAM_TYPE_BLOB,
            c74::max::PARAM_FLAGS_FORCE_TYPE);
        c74::max::object_attr_setlong(
            maxobj(),
            c74::max::gensym("parameter_enable"),
            3);
        c74::max::object_attr_setlong(
            maxobj(),
            c74::max::gensym("parameter_visibility"),
            1);
        parameterInitialized_ = error == c74::max::MAX_ERR_NONE;
    }

    if (!managed_.IsLoaded())
    {
        cerr
            << "Consolidator: failed to load managed core."
            << endl;

        return;
    }

    instanceId_ = managed_.RegisterInstance(
        this,
        &ConsolidatorExternal::ManagedOutputCallbackHandler,
        &dspExchange_,
        &audioInputHandle_);

    if (instanceId_ == 0)
    {
        cerr
            << "Consolidator: failed to register managed instance."
            << endl;
    }
}

ConsolidatorExternal::~ConsolidatorExternal()
{
    // Max stops new audio callbacks before destroying the external.
    if (instanceId_ != 0 &&
        managed_.IsLoaded())
    {
        managed_.UnregisterInstance(instanceId_);
    }

    instanceId_ = 0;
    audioInputHandle_ = 0;

    outputQueue_.unset();

    if (parameterInitialized_)
    {
        c74::max::object_parameter_free(maxobj());
        parameterInitialized_ = false;
    }
}

void ConsolidatorExternal::ForwardMessage(
    const char* selector,
    const atoms& atoms)
{
    if (instanceId_ == 0)
    {
        return;
    }

    const auto encoded = AtomCodec::Encode(atoms);

    if (!encoded)
    {
        cerr
            << "Consolidator: unsupported Max atom."
            << endl;

        return;
    }

    managed_.SendManagedMessage(
        instanceId_,
        selector,
        encoded->data(),
        encoded->size());
}

void __cdecl ConsolidatorExternal::ManagedOutputCallbackHandler(
    void* context,
    const char* selector,
    const NativeAtom* atoms,
    std::size_t atomCount) noexcept
{
    if (context == nullptr)
    {
        return;
    }

    try
    {
        static_cast<ConsolidatorExternal*>(context)
            ->ReceiveManagedOutput(selector, atoms, atomCount);
    }
    catch (...)
    {
    }
}

void ConsolidatorExternal::ReceiveManagedOutput(
    const char* selector,
    const NativeAtom* atoms,
    std::size_t atomCount)
{
    if (selector == nullptr || (atomCount != 0 && atoms == nullptr))
    {
        return;
    }

    if (std::strcmp(selector, "persistence_dirty") == 0)
    {
        persistenceDirty_.store(true, std::memory_order_relaxed);
        outputQueue_.set();
        return;
    }

    OutputFrame frame;
    frame.selector = selector;
    frame.atoms.reserve(atomCount);

    for (std::size_t index = 0; index < atomCount; ++index)
    {
        const auto& atom = atoms[index];

        switch (atom.type)
        {
        case NativeAtomType::Integer:
            frame.atoms.emplace_back(atom.integer);
            break;

        case NativeAtomType::Float:
            frame.atoms.emplace_back(atom.floating);
            break;

        case NativeAtomType::Symbol:
            frame.atoms.emplace_back(
                atom.symbol != nullptr ? atom.symbol : "");
            break;
        }
    }

    {
        std::lock_guard lock{ outputMutex_ };

        if (frame.selector == "fft")
        {
            if (frame.atoms.size() > 1)
            {
                if (std::holds_alternative<std::int64_t>(frame.atoms[1]))
                {
                    if (pendingFftFrame_)
                    {
                        replacedFftFrames_.fetch_add(1, std::memory_order_relaxed);
                    }
                    pendingFftFrame_ = std::move(frame);
                }
                else
                {
                    skippedFftFrames_.fetch_add(1, std::memory_order_relaxed);
                }
            }
            else
            {
                skippedFftFrames_.fetch_add(1, std::memory_order_relaxed);
            }
        }
        else
        {
            pendingControl_.push_back(std::move(frame));
            controlQueueDepth_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    outputQueue_.set();
}

void ConsolidatorExternal::DrainManagedOutput()
{
    const auto drainStarted = std::chrono::steady_clock::now();
    std::deque<OutputFrame> controlFrames;
    std::optional<OutputFrame> fftFrame;
    std::size_t controlAtomCount = 0;

    {
        std::lock_guard lock{ outputMutex_ };

        while (!pendingControl_.empty() &&
            controlFrames.size() < kMaxControlFramesPerDrain)
        {
            const auto& frame = pendingControl_.front();
            if (!controlFrames.empty() &&
                controlAtomCount + frame.atoms.size() >
                    kMaxControlAtomsPerDrain)
            {
                break;
            }

            controlAtomCount += frame.atoms.size();
            controlFrames.push_back(std::move(pendingControl_.front()));
            pendingControl_.pop_front();
            controlQueueDepth_.fetch_sub(1, std::memory_order_relaxed);
        }

        fftFrame.swap(pendingFftFrame_);
    }

    const auto sendFrame = [this](OutputFrame& frame)
    {
        atoms output;
        output.reserve(frame.atoms.size() + 1);
        output.emplace_back(symbol(frame.selector));

        for (const auto& value : frame.atoms)
        {
            std::visit(
                [&output](const auto& typedValue)
                {
                    using Value = std::decay_t<decltype(typedValue)>;

                    if constexpr (std::is_same_v<Value, std::int64_t>)
                    {
                        output.emplace_back(
                            static_cast<c74::max::t_atom_long>(typedValue));
                    }
                    else if constexpr (std::is_same_v<Value, double>)
                    {
                        output.emplace_back(typedValue);
                    }
                    else
                    {
                        output.emplace_back(symbol(typedValue));
                    }
                },
                value);
        }

        if (frame.selector == "fft")
        {
            analysisOutput.send(output);
        }
        else
        {
            controlOutput.send(output);
        }
    };

    if (persistenceDirty_.exchange(false, std::memory_order_relaxed))
    {
        c74::max::object_parameter_value_changed(
            maxobj(),
            1);
    }

    for (auto& frame : controlFrames)
    {
        sendFrame(frame);
    }

    if (fftFrame)
    {
        sendFrame(*fftFrame);
    }

    bool hasPendingWork = false;
    {
        std::lock_guard lock{ outputMutex_ };
        hasPendingWork = !pendingControl_.empty() ||
            pendingFftFrame_.has_value();
    }

    if (hasPendingWork)
    {
        outputQueue_.set();
    }

    lastDrainMicroseconds_.store(
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - drainStarted)
                .count()),
        std::memory_order_relaxed);
}

void ConsolidatorExternal::ReportMetrics() const
{
    c74::max::post(
        "Consolidator metrics: instance=%llu control_depth=%zu replaced_fft=%llu skipped_fft=%llu drain_us=%llu",
        static_cast<unsigned long long>(instanceId_),
        controlQueueDepth_.load(std::memory_order_relaxed),
        static_cast<unsigned long long>(
            replacedFftFrames_.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            skippedFftFrames_.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            lastDrainMicroseconds_.load(std::memory_order_relaxed)));
}

void ConsolidatorExternal::Prepare(
    const atoms& atoms)
{
    if (instanceId_ == 0 ||
        atoms.empty())
    {
        return;
    }

    const auto sampleRate = static_cast<double>(atoms[0]);
    dspParameterSmoother_.Prepare(sampleRate);

    managed_.Prepare(
        instanceId_,
        sampleRate,
        0);
}

void ConsolidatorExternal::operator()(
    audio_bundle input,
    audio_bundle output)
{
    const auto frameCount =
        static_cast<std::size_t>(
            input.frame_count());

    ConsumeDspState();

    // Managed copies analyzer input into its preallocated capture ring.
    if (instanceId_ != 0)
    {
        managed_.SendAudio(
            audioInputHandle_,
            input.samples(0),
            input.samples(1),
            input.samples(2),
            input.samples(3),
            frameCount);
    }

    std::copy_n(
        input.samples(0),
        frameCount,
        output.samples(0));

    std::copy_n(
        input.samples(1),
        frameCount,
        output.samples(1));

    std::copy_n(
        input.samples(2),
        frameCount,
        output.samples(2));

    std::copy_n(
        input.samples(3),
        frameCount,
        output.samples(3));

    ApplyDspRamp(
        output.samples(0),
        output.samples(1),
        frameCount);
}

void ConsolidatorExternal::ConsumeDspState() noexcept
{
    if (!ConsumePublishedDspState(
            dspExchange_,
            consumerDspIndex_,
            dspState_))
    {
        return;
    }

    dspParameterSmoother_.SetTarget(dspState_);
}

void ConsolidatorExternal::ApplyDspRamp(
    double* mainLeft,
    double* mainRight,
    std::size_t frameCount) noexcept
{
    for (std::size_t index = 0; index < frameCount; ++index)
    {
        const auto& parameters = dspParameterSmoother_.Advance();

        mainLeft[index] *= parameters.gain;
        mainRight[index] *= parameters.gain;
    }
}


} // namespace consolidator::max

MIN_EXTERNAL(consolidator::max::ConsolidatorExternal);

