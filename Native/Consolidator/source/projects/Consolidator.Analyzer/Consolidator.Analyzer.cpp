#include "c74_min.h"

#include "AnalyzerCurveBatch.h"
#include "AnalyzerCurveFrame.h"
#include "AnalyzerFilterVisuals.h"
#include "AnalyzerFrameBuffer.h"
#include "AnalyzerSpectrumEngine.h"
#include "AnalyzerWorkProcessor.h"
#include "Analysis/AnalyzerFeaturePipeline.h"
#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "EventCodec.h"
#include "SnapshotCodec.h"
#include "Settings/AnalysisOptions.h"
#include "Settings/SpectrumOptions.h"
#include "LatestValueTripleBuffer.h"
#include "Workflows/LatestWorkflowExecutor.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>

using namespace c74::min;
using namespace consolidator;

struct AnalyzerAsyncRuntime final {
    explicit AnalyzerAsyncRuntime(queue<>& completionQueue)
        : executor(
            [this](const std::shared_ptr<AnalyzerWorkTask>& task,
                   const workflows::WorkflowCancellation& cancellation) {
                return processor.Process(*task, cancellation);
            },
            [&completionQueue] { completionQueue.set(); }) {}

    AnalyzerWorkProcessor processor;
    workflows::LatestWorkflowExecutor<std::shared_ptr<AnalyzerWorkTask>, AnalyzerWorkResult> executor;
    dspcore::LatestValueTripleBuffer<AnalyzerWorkTask> tasks;
};

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
    inlet<> commandsIn{ this, "(message) inputs: snapshot 1 host eq <revision> <selectedBank> <bankCount> <banks...>; event 1 host <eventId> operation.changed analyzer ...; analyzer.view_changed <visible> <spectrum|analysis>; eq_preview <bankId> <filterId> <parameterIndex> <absoluteValue>" };
    inlet<> telemetryIn{
        this,
        "(anything) processor_telemetry <compressorReductionDb> <saturationNonlinearRatio> <saturationLevelDeltaDb> <inputPreDb> <inputPostDb> <outputPreDb> <outputPostDb> <compressorOutputDb> <saturatorOutputDb>"
    };

    outlet<> currentOut{ this, "(anything) current spectrum dB; clear_spectrum on silence" };
    outlet<> referenceOut{ this, "(anything) reference spectrum dB; clear_spectrum on silence" };
    outlet<> differenceOut{ this, "(anything) difference <dB...>; fit_curve <dB...>; clear_fit_curve" };
    outlet<> filterOut{
        this,
        "(anything) curve_settings <minimumHz> <maximumHz> <pointCount> <minimumSpectrumDb> <maximumSpectrumDb>; filter_curve <filterId> <active> <frequencyHz> <gainDb> <type> <q> [<curve...>]"
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

    queue<> analysisDispatch{
        this,
        MIN_FUNCTION {
            DispatchAnalysis();
            return {};
        }
    };

    queue<> analysisCompletion{
        this,
        MIN_FUNCTION {
            DeliverAnalysis();
            return {};
        }
    };

    queue<> analysisRuntimeInitialization{
        this,
        MIN_FUNCTION {
            InitializeAnalysisRuntime();
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

    message<> coordinatorTarget{
        this,
        "coordinator_target",
        "Reset EQ visual revision tracking before a new editing target snapshot",
        MIN_FUNCTION {
            if (inlet != 6 || args.size() != 2) return {};
            latestEqRevision = 0;
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

    message<> eqPreview{
        this,
        "eq_preview",
        "Apply one local EQ visual preview",
        MIN_FUNCTION {
            if (inlet != 6 || args.size() != 4) return {};
            if (!IsSpectrumViewActive()) return {};
            const auto bankId = static_cast<long>(args[0]);
            const auto filterId = static_cast<long>(args[1]);
            const auto parameterIndex = static_cast<long>(args[2]);
            const auto value = static_cast<double>(args[3]);
            const auto selected = filterVisuals.Snapshot().SelectedBank();
            if (!selected || bankId != selected->bankId || filterId < 1 || parameterIndex < 0) return {};
            const auto definition = filterVisuals.Definition(filterId);
            if (!definition || static_cast<std::size_t>(parameterIndex) >= definition->parameters.size()) return {};
            if (filterVisuals.UpdateParameter(bankId, filterId,
                definition->parameters[static_cast<std::size_t>(parameterIndex)].name, value)) {
                ScheduleFilterVisualDelivery();
            }
            return {};
        }
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
                const auto sampleRate = static_cast<double>(args[0]);
                analysisSampleRate.store(sampleRate, std::memory_order_release);
                analysisRuntimeInitialization.set();
                filterVisuals.SetSampleRate(sampleRate);
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
        auto* runtime = analysisRuntime.get();
        if (!runtime) return {};

        const consolidator::audio::AnalyzerInputFrame visualFrame{
            { currentLeftIn, currentRightIn },
            { referenceLeftIn, referenceRightIn }
        };
        const consolidator::audio::AnalyzerInputFrame fitInputFrame{
            { eqInputLeftIn, eqInputRightIn },
            { referenceLeftIn, referenceRightIn }
        };

        const auto viewVisible = analyzerViewVisible.load(std::memory_order_acquire);
        const auto viewMode = analyzerViewMode.load(std::memory_order_acquire);
        const auto sendSpectrum = viewVisible && viewMode == domain::AnalyzerViewMode::Spectrum;
        const auto sendAnalysis = viewVisible && viewMode == domain::AnalyzerViewMode::Analysis;
        const auto visualCaptureRequested = sendSpectrum || sendAnalysis;

        auto& task = runtime->tasks.ProducerValue();
        if (!task.collecting) {
            task.visualFrame.Reset();
            task.fitFrame.Reset();
            task.collecting = true;
        }

        if (visualCaptureRequested != visualCaptureEnabled) {
            task.visualFrame.Reset();
            visualCaptureEnabled = visualCaptureRequested;
        }
        if (visualCaptureRequested) task.visualFrame.Write(visualFrame);
        task.fitFrame.Write(fitInputFrame);

        const auto visualReady = visualCaptureRequested && task.visualFrame.Advance();
        const auto fitReady = task.fitFrame.Advance();
        if (visualReady || fitReady) {
            task.visualReady = visualReady;
            task.fitReady = fitReady;
            task.sendSpectrum = sendSpectrum;
            task.sendAnalysis = sendAnalysis;
            task.resetDifference = differenceResetRequested.exchange(false, std::memory_order_acq_rel);
            task.differenceGeneration = differenceGeneration.load(std::memory_order_acquire);
            task.sampleRate = analysisSampleRate.load(std::memory_order_acquire);
            task.gainLevels = ReadGainLevels();
            task.collecting = false;
            runtime->tasks.Publish();
            ScheduleAnalysisDispatch();
        }

        return {};
    }

private:
    void PublishCurveSettings() {
        filterOut.send(
            "curve_settings",
            consolidator::settings::SpectrumOptions::MinimumFrequencyHz,
            consolidator::settings::SpectrumOptions::MaximumFrequencyHz,
            static_cast<long>(consolidator::settings::AnalysisOptions::DefaultCurvePointCount),
            consolidator::settings::SpectrumOptions::MinimumSpectrumDb,
            consolidator::settings::SpectrumOptions::MaximumSpectrumDb);
    }

    void ResetDifferenceAccumulation() {
        differenceGeneration.fetch_add(1, std::memory_order_acq_rel);
        differenceResetRequested.store(true, std::memory_order_release);
        if (analysisRuntime) {
            analysisRuntime->executor.Cancel();
            analysisRuntime->tasks.DiscardLatest();
        }
        curveFrames.DiscardLatest();
        differenceOut.send("clear_fit_curve");
    }

    void InitializeAnalysisRuntime() {
        if (analysisRuntime) return;
        analysisRuntime = std::make_unique<AnalyzerAsyncRuntime>(analysisCompletion);
    }

    void DispatchAnalysis() {
        analysisDispatchScheduled.store(false, std::memory_order_release);
        if (!analysisRuntime) return;
        analysisRuntime->tasks.ConsumeLatest([this](const AnalyzerWorkTask& task) {
            auto input = std::make_shared<AnalyzerWorkTask>(task);
            analysisRuntime->executor.Submit(++analysisTaskRevision, std::move(input));
        });

        if (analysisRuntime->tasks.HasPending()) ScheduleAnalysisDispatch();
    }

    void DeliverAnalysis() {
        if (!analysisRuntime) return;
        auto completion = analysisRuntime->executor.TakeCompletion();
        if (!completion || completion->error || !completion->result || !completion->result->publish) return;

        curveFrames.ProducerValue() = std::move(completion->result->frame);
        curveFrames.Publish();
        ScheduleCurveDelivery();
    }

    void ScheduleAnalysisDispatch() {
        if (!analysisDispatchScheduled.exchange(true, std::memory_order_acq_rel)) {
            analysisDispatch.set();
        }
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
            if (!IsSpectrumViewActive()) return;
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
        if (IsSpectrumViewActive()) ScheduleFilterVisualDelivery();
    }

    void SetViewState(bool visible, domain::AnalyzerViewMode mode) {
        const auto visibleChanged = analyzerViewVisible.exchange(visible, std::memory_order_acq_rel) != visible;
        const auto modeChanged = analyzerViewMode.exchange(mode, std::memory_order_acq_rel) != mode;
        const auto changed = visibleChanged || modeChanged;
        if (!changed) return;
        if (analysisRuntime) {
            analysisRuntime->executor.Cancel();
            analysisRuntime->tasks.DiscardLatest();
        }
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
        result.referenceDb = -120.0;
        return result;
    }

    static constexpr double FilterVisualFrameIntervalMilliseconds = 16.0;

    std::unique_ptr<AnalyzerAsyncRuntime> analysisRuntime;
    dspcore::LatestValueTripleBuffer<AnalyzerCurveFrame> curveFrames;
    AnalyzerFilterVisuals filterVisuals;
    std::atomic<bool> analysisDispatchScheduled{ false };
    std::atomic<bool> curveDeliveryScheduled{ false };
    std::atomic<double> analysisSampleRate{ settings::AudioOptions::DefaultSampleRateHz };
    std::uint64_t analysisTaskRevision = 0;
    bool filterVisualDeliveryScheduled = false;
    bool filterVisualsDirty = false;
    bool visualCaptureEnabled = false;
    bool hostReadyPublished = false;
    domain::StoreRevision latestEqRevision = 0;
    std::atomic<bool> differenceResetRequested{ false };
    std::atomic<std::uint64_t> differenceGeneration{ 0 };
    std::atomic<bool> analyzerViewVisible{ false };
    std::atomic<domain::AnalyzerViewMode> analyzerViewMode{ domain::AnalyzerViewMode::Spectrum };
    std::atomic<double> inputPreDb{ -120.0 };
    std::atomic<double> inputPostDb{ -120.0 };
    std::atomic<double> outputPreDb{ -120.0 };
    std::atomic<double> outputPostDb{ -120.0 };
};

MIN_EXTERNAL_CUSTOM(ConsolidatorAnalyzer, consolidator.analyzer);
