#pragma once

#include <deque>
#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "AtomCodec.h"
#include "DspParameterSmoother.h"
#include "DspStateConsumer.h"
#include "ManagedBridge.h"
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
};

} // namespace consolidator::max
