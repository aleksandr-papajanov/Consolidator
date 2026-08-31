#pragma once

#include <atomic>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "AtomCodec.h"
#include "DspParameterSmoother.h"
#include "DspStateConsumer.h"
#include "ManagedBridge.h"
#include "PersistenceBlobCodec.h"
#include "SharedDspState.h"
#include "c74_min_api.h"

namespace consolidator::max
{

class ConsolidatorExternal
    : public c74::min::object<ConsolidatorExternal>,
      public c74::min::vector_operator<>
{
public:
    MIN_DESCRIPTION {
        "Consolidator Max bridge."
    };

    MIN_TAGS {
        "audio"
    };

    MIN_AUTHOR {
        "Consolidator"
    };

    c74::min::inlet<> controlInput{
        this,
        "Control messages."
    };

    c74::min::inlet<> mainInputLeft{
        this,
        "Main left.",
        "signal"
    };

    c74::min::inlet<> mainInputRight{
        this,
        "Main right.",
        "signal"
    };

    c74::min::inlet<> referenceInputLeft{
        this,
        "Reference left.",
        "signal"
    };

    c74::min::inlet<> referenceInputRight{
        this,
        "Reference right.",
        "signal"
    };

    c74::min::outlet<> controlOutput{
        this,
        "Control output."
    };

    c74::min::outlet<> analysisOutput{
        this,
        "Analysis output."
    };

    c74::min::outlet<> mainOutputLeft{
        this,
        "Main left.",
        "signal"
    };

    c74::min::outlet<> mainOutputRight{
        this,
        "Main right.",
        "signal"
    };

    c74::min::outlet<> referenceOutputLeft{
        this,
        "Reference left.",
        "signal"
    };

    c74::min::outlet<> referenceOutputRight{
        this,
        "Reference right.",
        "signal"
    };

    c74::min::attribute<c74::min::numbers> value{
        this,
        "value",
        c74::min::numbers{ 0.0 },
        c74::min::getter {
            [this]() -> c74::min::atoms
            {
                const auto snapshot = managed_.CapturePersistence(instanceId_);
                const auto encoded = PersistenceBlobCodec::Encode(snapshot);
                if (!encoded)
                {
                    return { 0.0 };
                }

                c74::min::atoms result;
                result.reserve(encoded->size());
                for (const auto atom : *encoded)
                {
                    result.emplace_back(atom);
                }
                return result;
            }
        },
        c74::min::setter {
            MIN_FUNCTION
            {
                const auto isInitialValue = args.size() == 1 &&
                    ((args[0].a_type == c74::max::A_FLOAT &&
                        static_cast<double>(args[0]) == 0.0) ||
                    (args[0].a_type == c74::max::A_LONG &&
                        static_cast<c74::max::t_atom_long>(args[0]) == 0));
                if (isInitialValue)
                {
                    return { 0.0 };
                }
                if (instanceId_ == 0)
                {
                    return { 0.0 };
                }

                std::vector<double> packed;
                packed.reserve(args.size());
                for (const auto& atom : args)
                {
                    if (atom.a_type == c74::max::A_LONG)
                    {
                        packed.push_back(
                            static_cast<double>(
                                static_cast<c74::max::t_atom_long>(atom)));
                        continue;
                    }
                    if (atom.a_type != c74::max::A_FLOAT)
                    {
                        return { 0.0 };
                    }

                    packed.push_back(static_cast<double>(atom));
                }

                const auto payload = PersistenceBlobCodec::Decode(packed);
                if (!payload)
                {
                    return { 0.0 };
                }

                if (!managed_.RestorePersistence(
                        instanceId_,
                        payload->data(),
                        payload->size()))
                {
                    return { 0.0 };
                }

                persistenceDirty_.store(
                    false,
                    std::memory_order_relaxed);
                return args;
            }
        }
    };

    //
    // Пока оставляем явные Max selectors.
    // Вся их реализация одна и та же.
    //

    c74::min::message<> initialize{
        this,
        "initialize",
        MIN_FUNCTION
        {
            ForwardMessage("initialize", args);
            return {};
        }
    };

    c74::min::message<> read{
        this,
        "read",
        MIN_FUNCTION
        {
            ForwardMessage("read", args);
            return {};
        }
    };

    c74::min::message<> write{
        this,
        "write",
        MIN_FUNCTION
        {
            ForwardMessage("write", args);
            return {};
        }
    };

    c74::min::message<> clearTopology{
        this,
        "clear_topology",
        MIN_FUNCTION
        {
            ForwardMessage("clear_topology", args);
            return {};
        }
    };

    c74::min::message<> reset{
        this,
        "reset",
        MIN_FUNCTION
        {
            ForwardMessage("reset", args);
            return {};
        }
    };

    c74::min::message<> registry{
        this,
        "registry",
        MIN_FUNCTION
        {
            ForwardMessage("registry", args);
            return {};
        }
    };

    c74::min::message<> observeTarget{
        this,
        "observe_target",
        MIN_FUNCTION
        {
            ForwardMessage("observe_target", args);
            return {};
        }
    };

    c74::min::message<> setInstanceActive{
        this,
        "set_instance_active",
        MIN_FUNCTION
        {
            ForwardMessage("set_instance_active", args);
            return {};
        }
    };

    c74::min::message<> setInstanceMute{
        this,
        "set_instance_mute",
        MIN_FUNCTION
        {
            ForwardMessage("set_instance_mute", args);
            return {};
        }
    };

    c74::min::message<> setInstanceSolo{
        this,
        "set_instance_solo",
        MIN_FUNCTION
        {
            ForwardMessage("set_instance_solo", args);
            return {};
        }
    };

    c74::min::message<> setProcessorBypass{
        this,
        "set_processor_bypass",
        MIN_FUNCTION
        {
            ForwardMessage("set_processor_bypass", args);
            return {};
        }
    };

    c74::min::message<> setProcessorSolo{
        this,
        "set_processor_solo",
        MIN_FUNCTION
        {
            ForwardMessage("set_processor_solo", args);
            return {};
        }
    };

    c74::min::message<> beginHistory{
        this,
        "begin_history",
        MIN_FUNCTION
        {
            ForwardMessage("begin_history", args);
            return {};
        }
    };

    c74::min::message<> endHistory{
        this,
        "end_history",
        MIN_FUNCTION
        {
            ForwardMessage("end_history", args);
            return {};
        }
    };

    c74::min::message<> jumpHistory{
        this,
        "jump_history",
        MIN_FUNCTION
        {
            ForwardMessage("jump_history", args);
            return {};
        }
    };

    c74::min::message<> metrics{
        this,
        "metrics",
        MIN_FUNCTION
        {
            ReportMetrics();
            ForwardMessage("metrics", args);
            return {};
        }
    };

    c74::min::message<> dspsetup{
        this,
        "dspsetup",
        MIN_FUNCTION
        {
            Prepare(args);
            return {};
        }
    };

    c74::min::message<> maxclassSetup{
        this,
        "maxclass_setup",
        MIN_FUNCTION
        {
            if (args.empty())
            {
                return {};
            }

            auto* maxClass = static_cast<c74::max::t_class*>(args[0]);
            c74::max::class_parameter_init(maxClass);
            return {};
        }
    };

    ConsolidatorExternal();
    ~ConsolidatorExternal();

    void operator()(
        c74::min::audio_bundle input,
        c74::min::audio_bundle output) override;

private:
    using OutputAtomValue = std::variant<std::int64_t, double, std::string>;

    struct OutputFrame
    {
        std::string selector;
        std::vector<OutputAtomValue> atoms;
    };

    static void __cdecl ManagedOutputCallbackHandler(
        void* context,
        const char* selector,
        const NativeAtom* atoms,
        std::size_t atomCount) noexcept;

    void ReceiveManagedOutput(
        const char* selector,
        const NativeAtom* atoms,
        std::size_t atomCount);

    void DrainManagedOutput();

    void ForwardMessage(
        const char* selector,
        const c74::min::atoms& atoms);

    void Prepare(
        const c74::min::atoms& atoms);

    void ConsumeDspState() noexcept;

    void ApplyDspRamp(
        double* mainLeft,
        double* mainRight,
        std::size_t frameCount) noexcept;

    void ReportMetrics() const;

    ManagedBridge managed_;
    InstanceId instanceId_{};
    AudioInputHandle audioInputHandle_{};
    SharedDspExchange dspExchange_{};
    DspSnapshot dspState_{};
    std::uint32_t consumerDspIndex_{};
    DspParameterSmoother dspParameterSmoother_;

    c74::min::queue<> outputQueue_{
        this,
        MIN_FUNCTION
        {
            DrainManagedOutput();
            return {};
        }
    };

    std::mutex outputMutex_;
    std::deque<OutputFrame> pendingControl_;
    std::optional<OutputFrame> pendingFftFrame_;
    std::atomic_size_t controlQueueDepth_{};
    std::atomic_uint64_t replacedFftFrames_{};
    std::atomic_uint64_t skippedFftFrames_{};
    std::atomic_uint64_t lastDrainMicroseconds_{};
    std::atomic_bool persistenceDirty_{};
    bool parameterInitialized_{ false };
};

} // namespace consolidator::max
