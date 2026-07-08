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

    inlet<> reference_l{ this, "(signal) reference left", "signal" };
    inlet<> reference_r{ this, "(signal) reference right", "signal" };
    inlet<> target_l{ this, "(signal) target left", "signal" };
    inlet<> target_r{ this, "(signal) target right", "signal" };

    outlet<> audio_l{ this, "(signal) passthrough left", "signal" };
    outlet<> audio_r{ this, "(signal) passthrough right", "signal" };

    outlet<> reference_out{ this, "(list) reference spectrum dB" };
    outlet<> target_out{ this, "(list) target spectrum dB" };
    outlet<> difference_out{ this, "(list) target-reference dB" };

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
        description { "Temporal smoothing amount for output curves. 0 = none, 0.98 = very slow." }
    };

    message<> dspsetup{ this, "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                spectrum_engine.set_sample_rate(static_cast<double>(args[0]));
            }

            return {};
        }
    };

    samples<2> operator()(sample ref_l, sample ref_r, sample eq_l, sample eq_r) {
        const int fft_size = spectrum_engine.sanitized_fft_size(fft_size_attr);
        const int bins_out = spectrum_engine.sanitized_detail(detail_attr, fft_size);
        const AnalyzerInputFrame frame{
            { ref_l, ref_r },
            { eq_l, eq_r }
        };

        input_stats.accumulate(frame);
        capture.write(frame);

        if (capture.advance(fft_size)) {
            spectrum_engine.analyze(
                capture,
                fft_size,
                bins_out,
                static_cast<double>(smoothing_attr),
                curves);

            capture.reset();
        }

        return { eq_l, eq_r };
    }

    message<> bang{ this, "bang", "Output latest analyzed curves.",
        MIN_FUNCTION {
            if (!curves.has_pending()) {
                return {};
            }

            curves.send(reference_out, target_out, difference_out);
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
