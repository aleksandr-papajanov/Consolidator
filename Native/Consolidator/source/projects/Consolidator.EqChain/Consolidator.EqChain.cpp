#include "c74_min.h"

#include "ComponentHost.h"
#include "DSP/Eq/EqRuntime.h"
#include "Settings/AudioOptions.h"

#include <atomic>
#include <memory>

using namespace c74::min;

class ConsolidatorEqChain :
    public object<ConsolidatorEqChain>,
    public sample_operator<2, 2> {
public:
    MIN_DESCRIPTION{ "Consolidator EQ chain audio processor." };
    MIN_TAGS{ "audio, eq, chain" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> inputLeft{ this, "(signal) left input", "signal" };
    inlet<> inputRight{ this, "(signal) right input", "signal" };
    inlet<> commandsIn{
        this,
        "(message) commands: message <dictionary type=device.state.changed>"
    };

    outlet<> outputLeft{ this, "(signal) left output", "signal" };
    outlet<> outputRight{ this, "(signal) right output", "signal" };
    outlet<> debugOut{ this, "(anything) diagnostics: error <code>" };

    message<> dspSetup{
        this,
        "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                sampleRate = static_cast<double>(args[0]);
                RebuildRuntime();
            }
            return {};
        }
    };

    message<> envelopeMessage{
        this,
        "message",
        "Apply a structured control envelope",
        MIN_FUNCTION {
            if (inlet != 2 || args.size() != 1) {
                debugOut.send("error", "invalid_message_envelope");
                return {};
            }
            component.Receive(args);
            return {};
        }
    };

    samples<2> operator()(sample left, sample right) {
        const auto runtime = runtimeState.load(std::memory_order_acquire);
        const auto output = runtime->chain.ProcessSample({ left, right });
        return { output.left, output.right };
    }

    void OnDeviceStateChanged(const consolidator::models::DeviceState& state) {
        eqRuntime.SetSnapshot(state.snapshot);
        RebuildRuntime();
    }

private:
    struct RuntimeState {
        consolidator::dsp::StereoDspChain chain;
    };

    void RebuildRuntime() {
        auto runtime = std::make_shared<RuntimeState>();
        runtime->chain = eqRuntime.BuildAllBanks(sampleRate).BuildStereo();
        runtimeState.store(std::move(runtime), std::memory_order_release);
    }

    double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz;
    consolidator::dsp::EqRuntime eqRuntime;
    consolidator::maxadapter::ComponentHost<ConsolidatorEqChain> component{
        *this, "eq.chain", nullptr, nullptr, &debugOut
    };
    std::atomic<std::shared_ptr<RuntimeState>> runtimeState{ std::make_shared<RuntimeState>() };
};

MIN_EXTERNAL_CUSTOM(ConsolidatorEqChain, consolidator.eqchain);
