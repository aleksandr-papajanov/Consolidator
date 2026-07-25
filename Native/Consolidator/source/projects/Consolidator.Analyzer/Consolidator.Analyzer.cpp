#include "c74_min.h"

#include "AnalyzerCurveBatch.h"
#include "AnalyzerCurveFrame.h"
#include "AnalyzerFilterVisuals.h"
#include "AnalyzerFrameBuffer.h"
#include "AnalyzerSpectrumEngine.h"
#include "Analysis/AnalyzerFeaturePipeline.h"
#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "EventCodec.h"
#include "SnapshotCodec.h"
#include "Settings/AnalysisOptions.h"
#include "Settings/SpectrumOptions.h"
#include "LatestValueTripleBuffer.h"

#include <atomic>
#include <cstdint>
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
    inlet<> telemetryIn{
        this,
        "(anything) processor_telemetry <compressorReductionDb> <saturationNonlinearRatio> <saturationLevelDeltaDb> <inputPreDb> <inputPostDb> <outputPreDb> <outputPostDb>"
    };

    outlet<> currentOut{ this, "(list) current spectrum dB" };
    outlet<> referenceOut{ this, "(list) reference spectrum dB" };
    outlet<> differenceOut{ this, "(anything) difference <dB...>; fit_curve <dB...>; clear_fit_curve" };
    outlet<> filterOut{
        this,
        "(anything) messages: curve_settings <minimumHz> <maximumHz> <pointCount>; filter_curve <filterId> <active> <frequencyHz> <gainDb> <type> <q> <qMin> <qMax> <curve...>"
    };
    outlet<> totalCurveOut{ this, "(list) summed response curve for all EQ banks in dB" };
    outlet<> statusOut{ this, "(anything) status: status initializing|ready|processing|error <code>" };
    outlet<> debugOut{ this, "(anything) diagnostics: error <code>" };
    outlet<> analysisOut{
        this,
        "(anything) visual analysis: feature_vector <windowCount> <historySeconds> <globalMetrics...> <bandMetrics...>"
    };
    outlet<> levelsOut{
        this,
        "(anything) gain_levels <inputPreDb> <inputPostDb> <outputPreDb> <outputPostDb> <referenceDb>"
    };

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

    message<> statusMessage{
        this,
        "status",
        "Ignore feature status messages on the shared command inlet",
        MIN_FUNCTION { return {}; }
    };

    message<> processorTelemetry{
        this,
        "processor_telemetry",
        "Store DSP gain-stage telemetry",
        MIN_FUNCTION {
            if (inlet != 5 || args.size() != 7) return {};
            inputPreDb.store(static_cast<double>(args[3]), std::memory_order_release);
            inputPostDb.store(static_cast<double>(args[4]), std::memory_order_release);
            outputPreDb.store(static_cast<double>(args[5]), std::memory_order_release);
            outputPostDb.store(static_cast<double>(args[6]), std::memory_order_release);
            return {};
        }
    };

    message<> dspsetup{ this, "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                spectrumEngine.SetSampleRate(static_cast<double>(args[0]));
                featurePipeline.SetSampleRate(static_cast<double>(args[0]));
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
            const auto frameDifferenceGeneration = differenceGeneration.load(std::memory_order_acquire);
            if (differenceResetRequested.exchange(false, std::memory_order_acq_rel)) {
                curves.ResetDifference();
            }
            const auto listenEnabled = differenceEnabled.load(std::memory_order_acquire);
            const auto spectra = spectrumEngine.Analyze(capture, curves, listenEnabled);
            auto featureFrame = featurePipeline.Process(capture, spectra);
            curves.WriteFrame(curveFrames.ProducerValue(), frameDifferenceGeneration);
            curveFrames.ProducerValue().SetFeatures(std::move(featureFrame));
            curveFrames.ProducerValue().SetGainLevels(ReadGainLevels());
            curveFrames.Publish();
            ScheduleCurveDelivery();

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
        if (differenceEnabled.exchange(enabled, std::memory_order_acq_rel) == enabled) return;
        ResetDifferenceAccumulation();
    }

    void ResetDifferenceAccumulation() {
        differenceGeneration.fetch_add(1, std::memory_order_acq_rel);
        differenceResetRequested.store(true, std::memory_order_release);
        curveFrames.DiscardLatest();
        differenceOut.send("clear_fit_curve");
    }

    void ApplyEvent(const std::optional<messaging::AtomList>& atoms) {
        if (!atoms) return;
        const auto decoded = messaging::EventCodec::Decode(*atoms);
        if (!decoded.Succeeded()) {
            debugOut.send("error", decoded.error.code);
            return;
        }
        const auto* operation = std::get_if<domain::OperationChangedEvent>(&decoded.event);
        if (!operation) return;
        if (operation->operation == "analyzer") {
            SetListenEnabled(operation->status == domain::OperationStatus::Capturing);
            return;
        }
        if (operation->operation.rfind("fit.", 0) == 0 &&
            operation->status == domain::OperationStatus::Completed &&
            differenceEnabled.load(std::memory_order_acquire)) {
            ResetDifferenceAccumulation();
        }
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
        curveFrames.ConsumeLatest([this](const AnalyzerCurveFrame& frame) {
            frame.Send(
                currentOut,
                referenceOut,
                differenceOut,
                filterVisuals.BanksThroughSelectedCurve(),
                analysisOut,
                levelsOut);
        });

        curveDeliveryScheduled.store(false, std::memory_order_release);
        if (curveFrames.HasPending()) ScheduleCurveDelivery();
    }

    void ScheduleCurveDelivery() {
        if (!curveDeliveryScheduled.exchange(true, std::memory_order_acq_rel)) {
            curveDelivery.set();
        }
    }

    audio::GainLevelMetrics ReadGainLevels() const noexcept {
        audio::GainLevelMetrics result;
        result.inputPreDb = inputPreDb.load(std::memory_order_acquire);
        result.inputPostDb = inputPostDb.load(std::memory_order_acquire);
        result.outputPreDb = outputPreDb.load(std::memory_order_acquire);
        result.outputPostDb = outputPostDb.load(std::memory_order_acquire);
        result.referenceDb = capture.ReferenceLevelDb();
        return result;
    }

    AnalyzerFrameBuffer capture;
    AnalyzerCurveBatch curves;
    dspcore::LatestValueTripleBuffer<AnalyzerCurveFrame> curveFrames;
    AnalyzerSpectrumEngine spectrumEngine;
    AnalyzerFeaturePipeline featurePipeline;
    AnalyzerFilterVisuals filterVisuals;
    std::atomic<bool> curveDeliveryScheduled{ false };
    std::atomic<bool> differenceResetRequested{ false };
    std::atomic<std::uint64_t> differenceGeneration{ 0 };
    std::atomic<bool> differenceEnabled{ false };
    std::atomic<double> inputPreDb{ -120.0 };
    std::atomic<double> inputPostDb{ -120.0 };
    std::atomic<double> outputPreDb{ -120.0 };
    std::atomic<double> outputPostDb{ -120.0 };
};

MIN_EXTERNAL_CUSTOM(ConsolidatorAnalyzer, consolidator.analyzer);
