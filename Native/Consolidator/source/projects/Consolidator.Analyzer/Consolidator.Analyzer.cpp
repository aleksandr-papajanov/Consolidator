#include "c74_min.h"

#include "AnalyzerCurveBatch.h"
#include "AnalyzerFilterVisuals.h"
#include "AnalyzerFrameBuffer.h"
#include "AnalyzerSpectrumEngine.h"
#include "AnalyzerStatistics.h"
#include "MessageFactory.h"
#include "TypedMessages.h"

using namespace c74::min;

class ConsolidatorAnalyzer :
    public object<ConsolidatorAnalyzer>,
    public sample_operator<4, 0> {
public:
    MIN_DESCRIPTION{ "Consolidator audio analyzer." };
    MIN_TAGS{ "audio, analyzer, fft" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> current_l{ this, "(signal) current left", "signal" };
    inlet<> current_r{ this, "(signal) current right", "signal" };
    inlet<> reference_l{ this, "(signal) reference left", "signal" };
    inlet<> reference_r{ this, "(signal) reference right", "signal" };
    inlet<> commands_in{ this, "(message) commands: message <dictionary type=analyzer.difference|analyzer.stats|filter.define|eq.storage.snapshot>" };

    outlet<> current_out{ this, "(list) current spectrum dB" };
    outlet<> reference_out{ this, "(list) reference spectrum dB" };
    outlet<> difference_out{ this, "(list) reference-current dB" };
    outlet<> filter_out{
        this,
        "(anything) messages: filter_curve <filterId> <active> <r> <g> <b> <a> <frequencyHz> <gainDb> <type> <q> <qMin> <qMax> <curve...>"
    };
    outlet<> total_curve_out{ this, "(list) summed response curve for all EQ banks in dB" };
    outlet<> selected_curve_out{ this, "(list) response curve for the selected EQ bank in dB" };
    outlet<> debug_out{ this, "(anything) diagnostics: error <code>" };

    queue<> curve_delivery{
        this,
        MIN_FUNCTION {
            publish_curves();
            return {};
        }
    };

    attribute<int> fft_size_attr{
        this,
        "fftsize",
        2048,
        range { 512, AnalyzerFrameBuffer::max_fft_size },
        description { "FFT analysis window size in samples. Must be power of two." }
    };

    attribute<double> smoothing_attr{
        this,
        "smoothing",
        0.75,
        range { 0.0, 0.98 },
        description { "Frequency-dependent temporal smoothing for output curves. Low frequencies are smoothed more strongly." }
    };

    attribute<double> low_frequency_amount_attr{
        this,
        "lowfreqsmoothing",
        1.0,
        range { 0.0, 1.0 },
        description { "Amount of extra smoothing applied to low frequencies." }
    };

    attribute<double> spectrum_calibration_db_attr{
        this,
        "spectrumcalibrationdb",
        24.0,
        range { -48.0, 48.0 },
        description { "Display calibration offset for the analyzer spectrum in dB." }
    };

    attribute<double> spectrum_tilt_db_attr{
        this,
        "spectrumtiltdb",
        0.0,
        range { -24.0, 24.0 },
        description { "Visual tilt of the analyzer spectrum in dB. Positive values lift high frequencies." }
    };

    message<> envelope_message{
        this,
        "message",
        "Apply a structured analyzer control envelope",
        MIN_FUNCTION {
            if (inlet != 4 || args.size() != 1) {
                debug_out.send("error", "invalid_message_envelope");
                return {};
            }
            auto message = consolidator::protocol::MessageFactory::from_atom(args[0]);
            if (!message) {
                debug_out.send("error", "invalid_message_envelope");
                return {};
            }
            if (!message->is_addressed_to("analyzer")) {
                return {};
            }
            const auto result = consolidator::protocol::dispatch<
                consolidator::protocol::AnalyzerDifferenceMessage,
                consolidator::protocol::AnalyzerStatsMessage,
                consolidator::protocol::FilterDefineMessage,
                consolidator::protocol::EqStorageSnapshotMessage>(*message, [this](const auto& command) {
                    handle_command(command);
                });
            if (result == consolidator::protocol::MessageDispatchResult::invalid) {
                debug_out.send("error", "invalid_message_envelope");
            }
            return {};
        }
    };

    message<> dspsetup{ this, "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                spectrum_engine.set_sample_rate(static_cast<double>(args[0]));
                filter_visuals.SetSampleRate(static_cast<double>(args[0]));
                filter_visuals.PublishSelected(filter_out);
                filter_visuals.PublishTotal(total_curve_out);
                filter_visuals.PublishSelectedBank(selected_curve_out);
            }

            return {};
        }
    };

    samples<0> operator()(sample current_l_in, sample current_r_in, sample reference_l_in, sample reference_r_in) {
        const int fft_size = spectrum_engine.sanitized_fft_size(fft_size_attr);
        const int bins_out = static_cast<int>(EqCurveGrid::point_count);
        const AnalyzerInputFrame frame{
            { current_l_in, current_r_in },
            { reference_l_in, reference_r_in }
        };

        input_stats.accumulate(frame);
        capture.write(frame);

        if (capture.advance(fft_size)) {
            spectrum_engine.analyze(
                capture,
                fft_size,
                bins_out,
                static_cast<double>(smoothing_attr),
                static_cast<double>(low_frequency_amount_attr),
                static_cast<double>(spectrum_calibration_db_attr),
                static_cast<double>(spectrum_tilt_db_attr),
                curves);
            curve_delivery.set();

            capture.reset();
        }

        return {};
    }

private:
    void handle_command(const consolidator::protocol::AnalyzerDifferenceMessage& command) {
        const bool state_changed = difference_enabled_ != command.enabled;
        difference_enabled_ = command.enabled;
        if (!difference_enabled_) {
            curves.clear_pending();
        }
        if (state_changed) {
            curves.ResetDifference();
        }
    }

    void handle_command(const consolidator::protocol::AnalyzerStatsMessage&) { publish_statistics(); }

    void handle_command(const consolidator::protocol::FilterDefineMessage& command) {
        if (!filter_visuals.Define(command)) {
            debug_out.send("error", "invalid_filter_visual_definition");
            return;
        }
        filter_visuals.PublishSelected(filter_out);
        filter_visuals.PublishTotal(total_curve_out);
        filter_visuals.PublishSelectedBank(selected_curve_out);
    }

    void handle_command(const consolidator::protocol::EqStorageSnapshotMessage& command) {
        if (!filter_visuals.SetSnapshot(command)) {
            debug_out.send(
                "error", "invalid_eq_storage_snapshot", filter_visuals.SnapshotError());
            return;
        }
        filter_visuals.PublishSelected(filter_out);
        filter_visuals.PublishTotal(total_curve_out);
        filter_visuals.PublishSelectedBank(selected_curve_out);
    }

    void publish_curves() {
        if (!curves.has_pending()) {
            return;
        }
        curves.send(
            current_out,
            reference_out,
            difference_out,
            difference_enabled_,
            filter_visuals.SelectedPrefixCurve());
        curves.clear_pending();
    }

    void publish_statistics() {
        difference_out.send(input_stats.build_atoms());
        input_stats.clear();
    }

    AnalyzerFrameBuffer capture;
    AnalyzerCurveBatch curves;
    AnalyzerStatistics input_stats;
    AnalyzerSpectrumEngine spectrum_engine;
    AnalyzerFilterVisuals filter_visuals;
    bool difference_enabled_ = true;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorAnalyzer, consolidator.analyzer);
