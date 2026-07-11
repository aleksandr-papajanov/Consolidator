#include "c74_min.h"

#include "AnalyzerSupport.h"

using namespace c74::min;

class ConsolidatorAnalyzer :
    public object<ConsolidatorAnalyzer>,
    public sample_operator<4, 2> {
public:
    MIN_DESCRIPTION{ "Consolidator audio analyzer." };
    MIN_TAGS{ "audio, analyzer, fft" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> current_l{ this, "(signal) current left", "signal" };
    inlet<> current_r{ this, "(signal) current right", "signal" };
    inlet<> reference_l{ this, "(signal) reference left", "signal" };
    inlet<> reference_r{ this, "(signal) reference right", "signal" };

    outlet<> audio_l{ this, "(signal) passthrough left", "signal" };
    outlet<> audio_r{ this, "(signal) passthrough right", "signal" };

    outlet<> current_out{ this, "(list) current spectrum dB" };
    outlet<> reference_out{ this, "(list) reference spectrum dB" };
    outlet<> difference_out{ this, "(list) reference-current dB" };

    attribute<int> fft_size_attr{
        this,
        "fftsize",
        2048,
        range { 512, AnalyzerFrameBuffer::max_fft_size },
        description { "FFT analysis window size in samples. Must be power of two." }
    };

    attribute<int> detail_attr{
        this,
        "detail",
        128,
        range { 32, AnalyzerCurveBatch::max_output_points },
        description { "Number of output points sent to the UI." }
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

    message<> dspsetup{ this, "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                spectrum_engine.set_sample_rate(static_cast<double>(args[0]));
            }

            return {};
        }
    };

    samples<2> operator()(sample current_l_in, sample current_r_in, sample reference_l_in, sample reference_r_in) {
        const int fft_size = spectrum_engine.sanitized_fft_size(fft_size_attr);
        const int bins_out = spectrum_engine.sanitized_detail(detail_attr, fft_size);
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

            capture.reset();
        }

        return { current_l_in, current_r_in };
    }

    message<> bang{ this, "bang", "Output latest analyzed curves.",
        MIN_FUNCTION {
            if (!curves.has_pending()) {
                return {};
            }

            curves.send(current_out, reference_out, difference_out);
            curves.clear_pending();

            return {};
        }
    };

    message<> stats_message{ this, "stats", "Output input RMS diagnostics.",
        MIN_FUNCTION {
            difference_out.send(input_stats.build_atoms());
            input_stats.clear();
            return {};
        }
    };

private:
    AnalyzerFrameBuffer capture;
    AnalyzerCurveBatch curves;
    AnalyzerStatistics input_stats;
    AnalyzerSpectrumEngine spectrum_engine;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorAnalyzer, consolidator.analyzer);
