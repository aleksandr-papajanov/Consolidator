#include "c74_min.h"

#include "AnalyzerCurveBatch.h"
#include "AnalyzerFilterVisuals.h"
#include "AnalyzerFrameBuffer.h"
#include "AnalyzerSpectrumEngine.h"
#include "MaxEqSnapshotAdapter.h"
#include "MaxFilterDefinitionAdapter.h"
#include "MaxMessageAdapter.h"
#include "Messaging/MessageRegistry.h"
#include "Messaging/Messages/AnalyzerDifferenceMessage.h"
#include "Messaging/Messages/EqSnapshotMessage.h"
#include "Messaging/Messages/FilterDefinitionMessage.h"

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
    inlet<> commands_in{ this, "(message) commands: message <dictionary type=analyzer.difference|filter.define|eq.storage.snapshot>" };

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

    message<> envelope_message{
        this,
        "message",
        "Apply a structured analyzer control envelope",
        MIN_FUNCTION {
            if (inlet != 4 || args.size() != 1) {
                debug_out.send("error", "invalid_message_envelope");
                return {};
            }
            const auto envelope = consolidator::maxadapter::MaxMessageAdapter::Deserialize(args[0]);
            if (!envelope) {
                debug_out.send("error", "invalid_message_envelope");
                return {};
            }
            if (!consolidator::maxadapter::MaxMessageAdapter::IsAddressedTo(*envelope, "analyzer")) {
                return {};
            }
            const auto command = messageFactory.Deserialize(*envelope);
            if (const auto* difference = dynamic_cast<consolidator::messaging::AnalyzerDifferenceMessage*>(command.get())) {
                handle_command(*difference);
            }
            else if (const auto* definition = dynamic_cast<consolidator::messaging::FilterDefinitionMessage*>(command.get())) {
                handle_command(*definition);
            }
            else if (const auto* snapshot = dynamic_cast<consolidator::messaging::EqSnapshotMessage*>(command.get())) {
                handle_command(*snapshot);
            }
            else {
                debug_out.send("error", "invalid_message_envelope");
            }
            return {};
        }
    };

    message<> dspsetup{ this, "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                spectrum_engine.SetSampleRate(static_cast<double>(args[0]));
                filter_visuals.SetSampleRate(static_cast<double>(args[0]));
                filter_visuals.PublishSelected(filter_out);
                filter_visuals.PublishTotal(total_curve_out);
                filter_visuals.PublishSelectedBank(selected_curve_out);
            }

            return {};
        }
    };

    samples<0> operator()(sample current_l_in, sample current_r_in, sample reference_l_in, sample reference_r_in) {
        const consolidator::audio::AnalyzerInputFrame frame{
            { current_l_in, current_r_in },
            { reference_l_in, reference_r_in }
        };

        capture.Write(frame);

        if (capture.Advance()) {
            spectrum_engine.Analyze(capture, curves);
            curve_delivery.set();

            capture.Reset();
        }

        return {};
    }

private:
    void handle_command(const consolidator::messaging::AnalyzerDifferenceMessage& command) {
        const bool state_changed = difference_enabled_ != command.enabled;
        difference_enabled_ = command.enabled;
        if (!difference_enabled_) {
            curves.ClearPending();
        }
        if (state_changed) {
            curves.ResetDifference();
        }
    }

    void handle_command(const consolidator::messaging::FilterDefinitionMessage& command) {
        const auto definition = consolidator::maxadapter::MaxFilterDefinitionAdapter::Read(
            command.contractName, command.filterId, command.defaultBypass);
        if (!definition) {
            debug_out.send("error", "invalid_filter_visual_definition");
            return;
        }
        filter_visuals.Define(*definition);
        filter_visuals.PublishSelected(filter_out);
        filter_visuals.PublishTotal(total_curve_out);
        filter_visuals.PublishSelectedBank(selected_curve_out);
    }

    void handle_command(const consolidator::messaging::EqSnapshotMessage& command) {
        const auto snapshot = consolidator::maxadapter::MaxEqSnapshotAdapter::Read(
            command.snapshotName, command.selectedBankId);
        if (!snapshot || !filter_visuals.SetSnapshot(*snapshot)) {
            debug_out.send(
                "error", "invalid_eq_storage_snapshot", filter_visuals.SnapshotError());
            return;
        }
        filter_visuals.PublishSelected(filter_out);
        filter_visuals.PublishTotal(total_curve_out);
        filter_visuals.PublishSelectedBank(selected_curve_out);
    }

    void publish_curves() {
        if (!curves.HasPending()) {
            return;
        }
        curves.Send(
            current_out,
            reference_out,
            difference_out,
            difference_enabled_,
            filter_visuals.SelectedPrefixCurve());
        curves.ClearPending();
    }

    AnalyzerFrameBuffer capture;
    AnalyzerCurveBatch curves;
    AnalyzerSpectrumEngine spectrum_engine;
    AnalyzerFilterVisuals filter_visuals;
    consolidator::messaging::MessageFactory messageFactory =
        consolidator::messaging::MessageRegistry::CreateFactory();
    bool difference_enabled_ = true;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorAnalyzer, consolidator.analyzer);
