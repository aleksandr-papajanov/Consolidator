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
    public sample_operator<6, 0> {
public:
    MIN_DESCRIPTION{ "Consolidator audio analyzer." };
    MIN_TAGS{ "audio, analyzer, fft" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> currentLeft{ this, "(signal) post-EQ current left", "signal" };
    inlet<> currentRight{ this, "(signal) post-EQ current right", "signal" };
    inlet<> referenceLeft{ this, "(signal) reference left", "signal" };
    inlet<> referenceRight{ this, "(signal) reference right", "signal" };
    inlet<> eqInputLeft{ this, "(signal) pre-EQ current left used by fit", "signal" };
    inlet<> eqInputRight{ this, "(signal) pre-EQ current right used by fit", "signal" };
    inlet<> commandsIn{ this, "(message) inputs: snapshot 1 host eq <revision> <selectedBank> <bankCount> <banks...>; event 1 host <eventId> operation.changed analyzer ...; analyzer.view_changed <visible> <spectrum|analysis>" };
    inlet<> telemetryIn{
        this,
        "(anything) processor_telemetry <compressorReductionDb> <saturationNonlinearRatio> <saturationLevelDeltaDb> <inputPreDb> <inputPostDb> <outputPreDb> <outputPostDb> <compressorOutputDb> <saturatorOutputDb>"
    };

    outlet<> currentOut{ this, "(anything) current spectrum dB; clear_spectrum on silence" };
    outlet<> referenceOut{ this, "(anything) reference spectrum dB; clear_spectrum on silence" };
    outlet<> differenceOut{ this, "(anything) difference <dB...>; fit_curve <dB...>; clear_fit_curve" };
    outlet<> filterOut{
        this,
        "(anything) messages: curve_settings <minimumHz> <maximumHz> <pointCount>; filter_curve <filterId> <active> <frequencyHz> <gainDb> <type> <q> <qMin> <qMax> <freqMin> <freqMax> <gainMin> <gainMax> <curve...>"
    };
    outlet<> totalCurveOut{ this, "(list) summed response curve for all EQ banks in dB" };
    outlet<> statusOut{ this, "(anything) status: status ready|host_ready" };
    outlet<> debugOut{ this, "(anything) diagnostics: error <code>" };
    outlet<> analysisOut{
        this,
        "(anything) visual analysis: feature_vector <windowCount> <historySeconds> <globalMetrics...> <bandMetrics...>; clear_analysis on silence"
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

    timer<timer_options::defer_delivery> filterVisualDelivery{
        this,
        MIN_FUNCTION {
            filterVisualDeliveryScheduled = false;
            if (!filterVisualsDirty) return {};
            filterVisualsDirty = false;
            if (!IsSpectrumViewActive()) return {};
            filterVisuals.RefreshCurves();
            filterVisuals.PublishSelected(filterOut);
            filterVisuals.PublishTotal(totalCurveOut);
            return {};
        }
    };

    message<> snapshotMessage{
        this,
        "snapshot",
        "Apply a complete EQ snapshot",
        MIN_FUNCTION {
            if (inlet != 6) {
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
            if (inlet != 6) return {};
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
            if (inlet != 6) return {};
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
            if (inlet != 7 || args.size() != 9) return {};
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
                if (IsSpectrumViewActive()) {
                    filterVisuals.RefreshCurves();
                    PublishCurveSettings();
                    filterVisuals.PublishSelected(filterOut);
                    filterVisuals.PublishTotal(totalCurveOut);
                }
                statusOut.send("status", "ready");
            }

            return {};
        }
    };

    samples<0> operator()(
        sample currentLeftIn,
        sample currentRightIn,
        sample referenceLeftIn,
        sample referenceRightIn,
        sample eqInputLeftIn,
        sample eqInputRightIn
    ) {
        const consolidator::audio::AnalyzerInputFrame frame{
            { currentLeftIn, currentRightIn },
            { referenceLeftIn, referenceRightIn }
        };
        const consolidator::audio::AnalyzerInputFrame fitFrame{
            { eqInputLeftIn, eqInputRightIn },
            { referenceLeftIn, referenceRightIn }
        };

        const auto viewVisible = analyzerViewVisible.load(std::memory_order_acquire);
        const auto viewMode = analyzerViewMode.load(std::memory_order_acquire);
        const auto sendSpectrum = viewVisible && viewMode == domain::AnalyzerViewMode::Spectrum;
        const auto sendAnalysis = viewVisible && viewMode == domain::AnalyzerViewMode::Analysis;

        capture.Write(frame);
        fitCapture.Write(fitFrame);

        const auto captureReady = capture.Advance();
        fitCapture.Advance();
        if (captureReady) {
            const auto frameDifferenceGeneration = differenceGeneration.load(std::memory_order_acquire);
            if (differenceResetRequested.exchange(false, std::memory_order_acq_rel)) {
                curves.ResetDifference();
                fitCurves.ResetDifference();
            }
            const auto visualActive = !capture.IsSilent();
            const auto fitActive = !fitCapture.IsReferenceSilent();
            if (visualActive || fitActive) {
                audioActive.store(true, std::memory_order_release);
                AnalyzerSpectrumResult spectra;
                if (visualActive && (sendSpectrum || sendAnalysis)) {
                    spectra = spectrumEngine.Analyze(capture, curves, sendSpectrum, false);
                }
                if (fitActive) {
                    AnalyzerSpectrumResult fitSpectra;
                    if (visualActive && (sendSpectrum || sendAnalysis)) {
                        spectrumEngine.AnalyzeCurrentWithReferenceInto(
                            fitCapture,
                            spectra.reference,
                            fitCurves,
                            fitSpectra,
                            true);
                    }
                    else {
                        spectrumEngine.Analyze(fitCapture, fitCurves, true, true);
                    }
                }
                if (sendSpectrum && fitActive) {
                    curves.WriteFrame(
                        curveFrames.ProducerValue(),
                        fitCurves,
                        frameDifferenceGeneration);
                }
                else if (sendSpectrum) {
                    curves.WriteFrame(curveFrames.ProducerValue(), frameDifferenceGeneration);
                }
                else if (fitActive) {
                    fitCurves.WriteFrame(curveFrames.ProducerValue(), frameDifferenceGeneration);
                }
                if (visualActive && sendAnalysis) {
                    curveFrames.ProducerValue().SetFeatures(featurePipeline.Process(capture, spectra));
                }
                curveFrames.ProducerValue().SetGainLevels(ReadGainLevels());
                curveFrames.Publish();
                ScheduleCurveDelivery();
            }
            else if (audioActive.exchange(false, std::memory_order_acq_rel)) {
                curveFrames.ProducerValue().MarkSilent();
                curveFrames.Publish();
                ScheduleCurveDelivery();
            }

            capture.Reset();
            fitCapture.Reset();
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
        if (const auto* view = std::get_if<domain::AnalyzerViewChangedEvent>(&decoded.event)) {
            SetViewState(view->visible, view->mode);
            return;
        }
        if (const auto* parameter = std::get_if<domain::ParameterUpdatedEvent>(&decoded.event)) {
            if (parameter->revision < latestEqRevision) return;
            if (parameter->device == "eq" && filterVisuals.UpdateParameter(
                parameter->bankId, parameter->filterId, parameter->parameter,
                parameter->value)) {
                latestEqRevision = parameter->revision;
                ScheduleFilterVisualDelivery();
            }
            return;
        }
        const auto* operation = std::get_if<domain::OperationChangedEvent>(&decoded.event);
        if (!operation) return;
        if (operation->operation == "analyzer.clear" &&
            operation->status == domain::OperationStatus::Completed) {
            ResetDifferenceAccumulation();
        }
    }

    void ApplySnapshot(const std::optional<messaging::AtomList>& atoms) {
        const auto revision = atoms && atoms->size() > 4
            ? std::get_if<std::int64_t>(&(*atoms)[4]) : nullptr;
        if (!revision || *revision < 0 ||
            static_cast<domain::StoreRevision>(*revision) < latestEqRevision) return;
        const auto snapshot = atoms ? messaging::SnapshotCodec::DecodeEq(*atoms) : std::nullopt;
        if (!snapshot || !filterVisuals.SetSnapshot(*snapshot)) {
            debugOut.send("error", "invalid_eq_snapshot");
            return;
        }
        latestEqRevision = static_cast<domain::StoreRevision>(*revision);
        if (!hostReadyPublished) {
            hostReadyPublished = true;
            statusOut.send("status", "host_ready");
        }
        ScheduleFilterVisualDelivery();
    }

    void SetViewState(bool visible, domain::AnalyzerViewMode mode) {
        const auto visibleChanged = analyzerViewVisible.exchange(visible, std::memory_order_acq_rel) != visible;
        const auto modeChanged = analyzerViewMode.exchange(mode, std::memory_order_acq_rel) != mode;
        const auto changed = visibleChanged || modeChanged;
        if (!changed) return;
        curveFrames.DiscardLatest();
        if (IsSpectrumViewActive()) {
            PublishCurveSettings();
            filterVisualsDirty = true;
            ScheduleFilterVisualDelivery();
        }
    }

    bool IsSpectrumViewActive() const noexcept {
        return analyzerViewVisible.load(std::memory_order_acquire) &&
            analyzerViewMode.load(std::memory_order_acquire) == domain::AnalyzerViewMode::Spectrum;
    }

    void ScheduleFilterVisualDelivery() {
        filterVisualsDirty = true;
        if (filterVisualDeliveryScheduled) return;
        filterVisualDeliveryScheduled = true;
        filterVisualDelivery.delay(FilterVisualFrameIntervalMilliseconds);
    }

    void PublishCurves() {
        curveFrames.ConsumeLatest([this](const AnalyzerCurveFrame& frame) {
            frame.Send(
                currentOut,
                referenceOut,
                differenceOut,
                filterVisuals.BanksThroughSelectedCurve(),
                analysisOut,
                levelsOut,
                IsSpectrumViewActive(),
                true,
                analyzerViewVisible.load(std::memory_order_acquire) &&
                    analyzerViewMode.load(std::memory_order_acquire) == domain::AnalyzerViewMode::Analysis,
                analyzerViewVisible.load(std::memory_order_acquire));
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

    static constexpr double FilterVisualFrameIntervalMilliseconds = 16.0;

    AnalyzerFrameBuffer capture;
    AnalyzerFrameBuffer fitCapture;
    AnalyzerCurveBatch curves;
    AnalyzerCurveBatch fitCurves;
    dspcore::LatestValueTripleBuffer<AnalyzerCurveFrame> curveFrames;
    AnalyzerSpectrumEngine spectrumEngine;
    AnalyzerFeaturePipeline featurePipeline;
    AnalyzerFilterVisuals filterVisuals;
    std::atomic<bool> curveDeliveryScheduled{ false };
    bool filterVisualDeliveryScheduled = false;
    bool filterVisualsDirty = false;
    bool hostReadyPublished = false;
    domain::StoreRevision latestEqRevision = 0;
    std::atomic<bool> differenceResetRequested{ false };
    std::atomic<std::uint64_t> differenceGeneration{ 0 };
    std::atomic<bool> audioActive{ false };
    std::atomic<bool> analyzerViewVisible{ false };
    std::atomic<domain::AnalyzerViewMode> analyzerViewMode{ domain::AnalyzerViewMode::Spectrum };
    std::atomic<double> inputPreDb{ -120.0 };
    std::atomic<double> inputPostDb{ -120.0 };
    std::atomic<double> outputPreDb{ -120.0 };
    std::atomic<double> outputPostDb{ -120.0 };
};

MIN_EXTERNAL_CUSTOM(ConsolidatorAnalyzer, consolidator.analyzer);
