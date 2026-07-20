#include "c74_min.h"

#include "AnalyzerCurveBatch.h"
#include "AnalyzerFilterVisuals.h"
#include "AnalyzerFrameBuffer.h"
#include "AnalyzerSpectrumEngine.h"
#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "EventCodec.h"
#include "SnapshotCodec.h"
#include "Settings/AnalysisOptions.h"
#include "Settings/SpectrumOptions.h"

#include <optional>

using namespace c74::min;
using namespace consolidator;

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
    inlet<> commandsIn{ this, "(message) inputs: snapshot 1 host eq <revision> <selectedBank> <bankCount> <banks...>; event 1 host <eventId> operation.changed analyzer ..." };

    outlet<> currentOut{ this, "(list) current spectrum dB" };
    outlet<> referenceOut{ this, "(list) reference spectrum dB" };
    outlet<> differenceOut{ this, "(list) reference-current dB" };
    outlet<> filterOut{
        this,
        "(anything) messages: curve_settings <minimumHz> <maximumHz> <pointCount>; filter_curve <filterId> <active> <frequencyHz> <gainDb> <type> <q> <qMin> <qMax> <curve...>"
    };
    outlet<> totalCurveOut{ this, "(list) summed response curve for all EQ banks in dB" };
    outlet<> statusOut{ this, "(anything) status: status initializing|ready|processing|error <code>" };
    outlet<> debugOut{ this, "(anything) diagnostics: error <code>" };

    queue<> curveDelivery{
        this,
        MIN_FUNCTION {
            PublishCurves();
            return {};
        }
    };

    message<> snapshotMessage{
        this,
        "snapshot",
        "Apply a complete EQ snapshot",
        MIN_FUNCTION {
            if (inlet != 4) {
                debugOut.send("error", "invalid_snapshot_inlet");
                return {};
            }
            auto atoms = maxadapter::AtomAdapter::Read(args);
            if (atoms) atoms->insert(atoms->begin(), "snapshot");
            if (messaging::AtomMessage::HasSnapshotStore(atoms, "eq")) ApplySnapshot(atoms);
            return {};
        }
    };

    message<> list{
        this,
        "list",
        "Receive a complete EQ snapshot atom list",
        MIN_FUNCTION {
            if (inlet != 4) return {};
            const auto atoms = maxadapter::AtomAdapter::Read(args);
            if (messaging::AtomMessage::HasSnapshotStore(atoms, "eq")) ApplySnapshot(atoms);
            else if (messaging::AtomMessage::HasCategory(atoms, "event")) ApplyEvent(atoms);
            return {};
        }
    };

    message<> eventMessage{
        this,
        "event",
        "Apply a Host operation event",
        MIN_FUNCTION {
            if (inlet != 4) return {};
            auto atoms = maxadapter::AtomAdapter::Read(args);
            if (atoms) atoms->insert(atoms->begin(), "event");
            ApplyEvent(atoms);
            return {};
        }
    };

    message<> dspsetup{ this, "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                spectrumEngine.SetSampleRate(static_cast<double>(args[0]));
                filterVisuals.SetSampleRate(static_cast<double>(args[0]));
                PublishCurveSettings();
                filterVisuals.PublishSelected(filterOut);
                filterVisuals.PublishTotal(totalCurveOut);
                statusOut.send("status", "ready");
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

private:
    void PublishCurveSettings() {
        filterOut.send(
            "curve_settings",
            consolidator::settings::SpectrumOptions::MinimumFrequencyHz,
            consolidator::settings::SpectrumOptions::MaximumFrequencyHz,
            static_cast<long>(consolidator::settings::AnalysisOptions::DefaultCurvePointCount));
    }

    void SetListenEnabled(bool enabled) {
        if (differenceEnabled == enabled) return;
        differenceEnabled = enabled;
        if (!differenceEnabled) {
            curves.ClearPending();
            differenceOut.send("clear_difference");
        }
        curves.ResetDifference();
    }

    void ApplyEvent(const std::optional<messaging::AtomList>& atoms) {
        if (!atoms) return;
        const auto decoded = messaging::EventCodec::Decode(*atoms);
        if (!decoded.Succeeded()) return;
        const auto* operation = std::get_if<domain::OperationChangedEvent>(&decoded.event);
        if (!operation || operation->operation != "analyzer") return;
        SetListenEnabled(operation->status == domain::OperationStatus::Capturing);
    }

    void ApplySnapshot(const std::optional<messaging::AtomList>& atoms) {
        const auto snapshot = atoms ? messaging::SnapshotCodec::DecodeEq(*atoms) : std::nullopt;
        if (!snapshot || !filterVisuals.SetSnapshot(*snapshot)) {
            debugOut.send("error", "invalid_eq_snapshot");
            return;
        }
        filterVisuals.PublishSelected(filterOut);
        filterVisuals.PublishTotal(totalCurveOut);
    }

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
    bool differenceEnabled = false;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorAnalyzer, consolidator.analyzer);
