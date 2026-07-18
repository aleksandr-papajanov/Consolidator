#include "c74_min.h"

#include "AnalyzerCurveBatch.h"
#include "AnalyzerFilterVisuals.h"
#include "AnalyzerFrameBuffer.h"
#include "AnalyzerSpectrumEngine.h"
#include "ComponentHost.h"
#include "Messaging/Messages/AnalyzerDifferenceMessage.h"

using namespace c74::min;

class ConsolidatorAnalyzer :
    public object<ConsolidatorAnalyzer>,
    public sample_operator<4, 0> {
public:
    MIN_DESCRIPTION{ "Consolidator audio analyzer." };
    MIN_TAGS{ "audio, analyzer, fft" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> currentLeft{ this, "(signal) current left", "signal" };
    inlet<> currentRight{ this, "(signal) current right", "signal" };
    inlet<> referenceLeft{ this, "(signal) reference left", "signal" };
    inlet<> referenceRight{ this, "(signal) reference right", "signal" };
    inlet<> commandsIn{ this, "(message) commands: message <dictionary type=analyzer.difference|device.state.changed>" };

    outlet<> currentOut{ this, "(list) current spectrum dB" };
    outlet<> referenceOut{ this, "(list) reference spectrum dB" };
    outlet<> differenceOut{ this, "(list) reference-current dB" };
    outlet<> filterOut{
        this,
        "(anything) messages: filter_curve <filterId> <active> <r> <g> <b> <a> <frequencyHz> <gainDb> <type> <q> <qMin> <qMax> <curve...>"
    };
    outlet<> totalCurveOut{ this, "(list) summed response curve for all EQ banks in dB" };
    outlet<> debugOut{ this, "(anything) diagnostics: error <code>" };

    queue<> curveDelivery{
        this,
        MIN_FUNCTION {
            PublishCurves();
            return {};
        }
    };

    message<> envelopeMessage{
        this,
        "message",
        "Apply a structured analyzer control envelope",
        MIN_FUNCTION {
            if (inlet != 4 || args.size() != 1) {
                debugOut.send("error", "invalid_message_envelope");
                return {};
            }
            component.Receive(args);
            return {};
        }
    };

    message<> dspsetup{ this, "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                spectrumEngine.SetSampleRate(static_cast<double>(args[0]));
                filterVisuals.SetSampleRate(static_cast<double>(args[0]));
                filterVisuals.PublishSelected(filterOut);
                filterVisuals.PublishTotal(totalCurveOut);
            }

            return {};
        }
    };

    samples<0> operator()(sample currentLeftIn, sample currentRightIn, sample referenceLeftIn, sample referenceRightIn) {
        const consolidator::audio::AnalyzerInputFrame frame{
            { currentLeftIn, currentRightIn },
            { referenceLeftIn, referenceRightIn }
        };

        capture.Write(frame);

        if (capture.Advance()) {
            spectrumEngine.Analyze(capture, curves);
            curveDelivery.set();

            capture.Reset();
        }

        return {};
    }

    void OnMessage(const consolidator::messaging::AnalyzerDifferenceMessage& command) {
        const bool stateChanged = differenceEnabled != command.enabled;
        differenceEnabled = command.enabled;
        if (!differenceEnabled) {
            curves.ClearPending();
        }
        if (stateChanged) {
            curves.ResetDifference();
        }
    }

    void OnDeviceStateChanged(const consolidator::models::DeviceState& state) {
        filterVisuals.ClearDefinitions();
        for (const auto& definition : state.filterDefinitions) filterVisuals.Define(definition);
        if (!filterVisuals.SetSnapshot(state.snapshot)) {
            debugOut.send("error", "invalid_device_state", filterVisuals.SnapshotError());
            return;
        }
        filterVisuals.PublishSelected(filterOut);
        filterVisuals.PublishTotal(totalCurveOut);
    }

private:
    void PublishCurves() {
        if (!curves.HasPending()) {
            return;
        }
        curves.Send(
            currentOut,
            referenceOut,
            differenceOut,
            differenceEnabled,
            filterVisuals.SelectedPrefixCurve());
        curves.ClearPending();
    }

    AnalyzerFrameBuffer capture;
    AnalyzerCurveBatch curves;
    AnalyzerSpectrumEngine spectrumEngine;
    AnalyzerFilterVisuals filterVisuals;
    consolidator::maxadapter::ComponentHost<
        ConsolidatorAnalyzer,
        consolidator::messaging::AnalyzerDifferenceMessage
    > component{ *this, "analyzer", nullptr, nullptr, &debugOut };
    bool differenceEnabled = false;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorAnalyzer, consolidator.analyzer);
