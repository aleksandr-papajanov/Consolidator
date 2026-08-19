#include "ConsolidatorExternal.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <utility>

#ifdef SendMessage
#undef SendMessage
#endif

namespace consolidator::max
{

using namespace c74::min;

ConsolidatorExternal::ConsolidatorExternal()
    : managed_(cerr)
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
        &dspExchange_);

    if (instanceId_ == 0)
    {
        cerr
            << "Consolidator: failed to register managed instance."
            << endl;
    }
}

ConsolidatorExternal::~ConsolidatorExternal()
{
    if (instanceId_ != 0 &&
        managed_.IsLoaded())
    {
        managed_.UnregisterInstance(instanceId_);
    }

    instanceId_ = 0;

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
        pendingOutput_.push_back(std::move(frame));
    }

    outputQueue_.set();
}

void ConsolidatorExternal::DrainManagedOutput()
{
    std::deque<OutputFrame> frames;

    {
        std::lock_guard lock{ outputMutex_ };
        frames.swap(pendingOutput_);
    }

    for (auto& frame : frames)
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

        controlOutput.send(output);
    }
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
            instanceId_,
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
    const auto publishedIndex =
        std::atomic_ref{ dspExchange_.publishedIndex }
            .load(std::memory_order_acquire);

    if (publishedIndex > 2 ||
        publishedIndex == consumerDspIndex_)
    {
        return;
    }

    std::atomic_ref{ dspExchange_.consumerIndex }
        .store(
            publishedIndex,
            std::memory_order_release);
    dspState_ = dspExchange_.snapshots[publishedIndex];
    consumerDspIndex_ = publishedIndex;
}

} // namespace consolidator::max

MIN_EXTERNAL(consolidator::max::ConsolidatorExternal);