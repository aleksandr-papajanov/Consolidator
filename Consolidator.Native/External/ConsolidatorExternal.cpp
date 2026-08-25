#include "ConsolidatorExternal.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
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

bool IsAnalysisSelector(const std::string& selector)
{
    return selector == "fft" ||
        selector == "equalizer_curves" ||
        selector == "compressor_detector_curves" ||
        selector == "saturator_detector_curves";
}

}

ConsolidatorExternal::ConsolidatorExternal()
    : managed_()
{
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
                if (const auto source = std::get_if<std::int64_t>(
                    &frame.atoms[1]))
                {
                    if (pendingFftBySource_.contains(*source))
                    {
                        replacedFftFrames_.fetch_add(1, std::memory_order_relaxed);
                    }
                    pendingFftBySource_[*source] = std::move(frame);
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
        else if (frame.selector == "equalizer_curves" ||
            frame.selector == "compressor_detector_curves" ||
            frame.selector == "saturator_detector_curves")
        {
            const auto selector = frame.selector;
            pendingCurveFrames_[selector] = std::move(frame);
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
    std::unordered_map<std::int64_t, OutputFrame> fftFrames;
    std::unordered_map<std::string, OutputFrame> curveFrames;
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

        fftFrames.swap(pendingFftBySource_);
        curveFrames.swap(pendingCurveFrames_);
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

        if (IsAnalysisSelector(frame.selector) ||
            frame.selector == "combined" ||
            frame.selector == "all_banks")
        {
            analysisOutput.send(output);
        }
        else
        {
            controlOutput.send(output);
        }
    };

    for (auto& frame : controlFrames)
    {
        sendFrame(frame);
    }

    for (auto& entry : fftFrames)
    {
        sendFrame(entry.second);
    }

    for (auto& entry : curveFrames)
    {
        sendFrame(entry.second);
    }

    bool hasPendingWork = false;
    {
        std::lock_guard lock{ outputMutex_ };
        hasPendingWork = !pendingControl_.empty() ||
            !pendingFftBySource_.empty() ||
            !pendingCurveFrames_.empty();
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

    //
    // Analyzer input.
    //
    // C# implementation пока должна либо ничего не делать,
    // либо очень быстро копировать данные в заранее подготовленный buffer.
    //
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

    //
    // Пока DSP нет — просто passthrough.
    //

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

    for (std::size_t index = 0; index < frameCount; ++index)
    {
        output.samples(0)[index] *= dspState_.gain;
        output.samples(1)[index] *= dspState_.gain;
    }
}

void ConsolidatorExternal::ConsumeDspState() noexcept
{
    static_cast<void>(ConsumePublishedDspState(
        dspExchange_,
        consumerDspIndex_,
        dspState_));
}

} // namespace consolidator::max

MIN_EXTERNAL(consolidator::max::ConsolidatorExternal);
