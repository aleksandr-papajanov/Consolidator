#pragma once

#include "AtomReader.h"
#include "AtomWriter.h"
#include "Commands/Commands.h"
#include "Settings/CompressorOptions.h"
#include "Settings/GainOptions.h"
#include "Settings/SaturatorOptions.h"

#include <optional>
#include <limits>
#include <string>
#include <utility>

namespace consolidator::messaging {

struct DecodedCommand {
    domain::Command command;
    ProtocolError error;

    bool Succeeded() const noexcept { return error.code.empty(); }
};

class CommandCodec final {
public:
    static DecodedCommand Decode(const AtomList& atoms) {
        AtomReader reader(atoms);
        const auto category = reader.ReadString();
        const auto version = reader.ReadInt();
        const auto source = reader.ReadString();
        const auto requestId = reader.ReadInt();
        const auto name = reader.ReadString();
        if (!category || !version || !source || !requestId || !name || *category != "command" ||
            *version != 1 || source->empty() || *requestId < 1 || name->empty()) {
            return Invalid("invalid_command_header", reader.Index());
        }

        if (*name == "eq.set_parameter") {
            const auto bankId = reader.ReadInt();
            const auto filterId = reader.ReadInt();
            const auto parameter = reader.ReadString();
            const auto value = reader.ReadDouble();
            if (!bankId || !filterId || !parameter || !value || *bankId < 1 || *filterId < 1 || parameter->empty() || !reader.RequireEnd()) return Invalid("invalid_eq_set_parameter", reader.Index());
            return Success(domain::SetEqParameterCommand{
                { *requestId }, { *bankId }, { *filterId }, *parameter, *value
            });
        }
        if (*name == "eq.set_parameter_index") {
            const auto bankId = reader.ReadInt();
            const auto filterId = reader.ReadInt();
            const auto parameterIndex = reader.ReadInt();
            const auto value = reader.ReadDouble();
            if (!bankId || !filterId || !parameterIndex || !value || *bankId < 1 || *filterId < 1 ||
                *parameterIndex < 0 || !reader.RequireEnd()) {
                return Invalid("invalid_eq_set_parameter_index", reader.Index());
            }
            return Success(domain::SetEqParameterIndexCommand{
                { *requestId }, { *bankId }, { *filterId }, static_cast<std::size_t>(*parameterIndex), *value
            });
        }
        if (*name == "eq.reset_filter") {
            const auto bankId = reader.ReadInt();
            const auto filterId = reader.ReadInt();
            if (!bankId || !filterId || *bankId < 1 || *filterId < 1 || !reader.RequireEnd()) return Invalid("invalid_eq_reset_filter", reader.Index());
            return Success(domain::ResetEqFilterCommand{
                { *requestId }, { *bankId }, { *filterId }
            });
        }
        if (*name == "eq.set_bypass") {
            const auto bankId = reader.ReadInt();
            const auto filterId = reader.ReadInt();
            const auto bypass = reader.ReadBool();
            if (!bankId || !filterId || !bypass || *bankId < 1 || *filterId < 1 || !reader.RequireEnd()) return Invalid("invalid_eq_set_bypass", reader.Index());
            return Success(domain::SetEqBypassCommand{
                { *requestId }, { *bankId }, { *filterId }, *bypass
            });
        }
        if (*name == "eq.set_chain_bypass") {
            const auto bypass = reader.ReadBool();
            if (!bypass || !reader.RequireEnd()) return Invalid("invalid_eq_set_chain_bypass", reader.Index());
            return Success(domain::SetEqChainBypassCommand{ { *requestId }, *bypass });
        }
        if (*name == "eq.set_chain_solo") {
            const auto solo = reader.ReadBool();
            if (!solo || !reader.RequireEnd()) return Invalid("invalid_eq_set_chain_solo", reader.Index());
            return Success(domain::SetEqChainSoloCommand{ { *requestId }, *solo });
        }
        if (*name == "eq.reset") {
            const auto bankId = reader.ReadInt();
            if (!bankId || *bankId < 1 || !reader.RequireEnd()) return Invalid("invalid_eq_reset", reader.Index());
            return Success(domain::ResetEqChainCommand{ { *requestId }, { *bankId } });
        }
        if (*name == "eq.reset_all") {
            return reader.RequireEnd()
                ? Success(domain::ResetAllEqBanksCommand{ { *requestId } })
                : Invalid("invalid_eq_reset_all", reader.Index());
        }
        if (*name == "eq.join_banks") {
            const auto count = reader.ReadInt();
            if (!count || *count < 1 || *count > 6) {
                return Invalid("invalid_eq_bank_selection", reader.Index());
            }
            std::vector<domain::BankId> bankIds;
            bankIds.reserve(static_cast<std::size_t>(*count));
            for (long index = 0; index < *count; ++index) {
                const auto bankId = reader.ReadInt();
                if (!bankId || *bankId < 1 || *bankId > 6) return Invalid("invalid_eq_bank_selection", reader.Index());
                bankIds.push_back({ *bankId });
            }
            if (!reader.RequireEnd()) return Invalid("invalid_eq_bank_selection", reader.Index());
            return Success(domain::JoinEqBanksCommand{ { *requestId }, std::move(bankIds) });
        }
        if (*name == "eq.commit_hidden") {
            const auto bankId = reader.ReadInt();
            if (!bankId || *bankId < 1 || *bankId > 6 || !reader.RequireEnd()) return Invalid("invalid_eq_commit_hidden", reader.Index());
            return Success(domain::CommitHiddenEqBankCommand{ { *requestId }, { *bankId } });
        }
        if (*name == "eq.commit_all") {
            return reader.RequireEnd()
                ? Success(domain::CommitAllEqBanksCommand{ { *requestId } })
                : Invalid("invalid_eq_commit_all", reader.Index());
        }
        if (*name == "eq.set_link") {
            const auto bankId = reader.ReadInt();
            const auto linkId = reader.ReadString();
            if (!bankId || !linkId || *bankId < 1 || *bankId > 6 || !reader.RequireEnd()) {
                return Invalid("invalid_eq_set_link", reader.Index());
            }
            return Success(domain::SetEqBankLinkCommand{
                { *requestId }, { *bankId }, *linkId == "-" ? std::string{} : *linkId
            });
        }
        if (*name == "eq.select_bank") {
            const auto bankId = reader.ReadInt();
            if (!bankId || *bankId < 1 || !reader.RequireEnd()) return Invalid("invalid_eq_select_bank", reader.Index());
            return Success(domain::SelectEqBankCommand{ { *requestId }, { *bankId } });
        }
        if (*name == "gain.set_parameter") {
            const auto stage = reader.ReadString();
            const auto gainDb = reader.ReadDouble();
            if (!stage || !gainDb || !reader.RequireEnd()) {
                return Invalid("invalid_gain_set_parameter", reader.Index());
            }
            domain::GainStage gainStage;
            if (*stage == "input") gainStage = domain::GainStage::Input;
            else if (*stage == "output") gainStage = domain::GainStage::Output;
            else return Invalid("invalid_gain_stage", reader.Index());
            return Success(domain::SetGainParameterCommand{ { *requestId }, gainStage, *gainDb });
        }
        if (*name == "compressor.set_parameter") {
            const auto parameter = reader.ReadString();
            const auto value = reader.ReadDouble();
            if (!parameter || !value || parameter->empty() || !reader.RequireEnd()) return Invalid("invalid_compressor_set_parameter", reader.Index());
            return Success(domain::SetCompressorParameterCommand{ { *requestId }, *parameter, *value });
        }
        if (*name == "compressor.set_bypass") {
            const auto bypass = reader.ReadBool();
            if (!bypass || !reader.RequireEnd()) return Invalid("invalid_compressor_set_bypass", reader.Index());
            return Success(domain::SetCompressorBypassCommand{ { *requestId }, *bypass });
        }
        if (*name == "compressor.set_detector_parameter") {
            const auto filterId = reader.ReadInt();
            const auto parameter = reader.ReadString();
            const auto value = reader.ReadDouble();
            if (!filterId || !parameter || !value || *filterId < 1 || *filterId > 2 || parameter->empty() || !reader.RequireEnd()) {
                return Invalid("invalid_compressor_detector_parameter", reader.Index());
            }
            return Success(domain::SetCompressorDetectorParameterCommand{ { *requestId }, static_cast<long>(*filterId), *parameter, *value });
        }
        if (*name == "compressor.set_detector_listen") {
            const auto filterId = reader.ReadInt();
            const auto enabled = reader.ReadBool();
            if (!filterId || !enabled || *filterId < 1 || *filterId > 2 || !reader.RequireEnd()) return Invalid("invalid_compressor_detector_listen", reader.Index());
            return Success(domain::SetCompressorDetectorListenCommand{ { *requestId }, static_cast<long>(*filterId), *enabled });
        }
        if (*name == "compressor.reset") return reader.RequireEnd()
            ? Success(domain::ResetCompressorCommand{ { *requestId } })
            : Invalid("invalid_compressor_reset", reader.Index());
        if (*name == "saturator.set_parameter") {
            const auto parameter = reader.ReadString();
            const auto value = reader.ReadDouble();
            if (!parameter || !value || parameter->empty() || !reader.RequireEnd()) return Invalid("invalid_saturator_set_parameter", reader.Index());
            return Success(domain::SetSaturatorParameterCommand{ { *requestId }, *parameter, *value });
        }
        if (*name == "saturator.set_bypass") {
            const auto bypass = reader.ReadBool();
            if (!bypass || !reader.RequireEnd()) return Invalid("invalid_saturator_set_bypass", reader.Index());
            return Success(domain::SetSaturatorBypassCommand{ { *requestId }, *bypass });
        }
        if (*name == "saturator.set_detector_parameter") {
            const auto filterId = reader.ReadInt();
            const auto parameter = reader.ReadString();
            const auto value = reader.ReadDouble();
            if (!filterId || !parameter || !value || *filterId < 1 || *filterId > 2 || parameter->empty() || !reader.RequireEnd()) {
                return Invalid("invalid_saturator_detector_parameter", reader.Index());
            }
            return Success(domain::SetSaturatorDetectorParameterCommand{ { *requestId }, static_cast<long>(*filterId), *parameter, *value });
        }
        if (*name == "saturator.set_detector_listen") {
            const auto filterId = reader.ReadInt();
            const auto enabled = reader.ReadBool();
            if (!filterId || !enabled || *filterId < 1 || *filterId > 2 || !reader.RequireEnd()) return Invalid("invalid_saturator_detector_listen", reader.Index());
            return Success(domain::SetSaturatorDetectorListenCommand{ { *requestId }, static_cast<long>(*filterId), *enabled });
        }
        if (*name == "saturator.reset") return reader.RequireEnd()
            ? Success(domain::ResetSaturatorCommand{ { *requestId } })
            : Invalid("invalid_saturator_reset", reader.Index());
        if (*name == "analyzer.clear") {
            if (!reader.RequireEnd()) return Invalid("invalid_analyzer_clear", reader.Index());
            return Success(domain::ClearAnalyzerCommand{ { *requestId } });
        }
        if (*name == "analyzer.set_view") {
            const auto visible = reader.ReadBool();
            const auto mode = reader.ReadString();
            if (!visible || !mode || !reader.RequireEnd()) {
                return Invalid("invalid_analyzer_set_view", reader.Index());
            }
            if (*mode == "spectrum") {
                return Success(domain::SetAnalyzerViewCommand{
                    { *requestId }, *visible, domain::AnalyzerViewMode::Spectrum
                });
            }
            if (*mode == "analysis") {
                return Success(domain::SetAnalyzerViewCommand{
                    { *requestId }, *visible, domain::AnalyzerViewMode::Analysis
                });
            }
            return Invalid("invalid_analyzer_view_mode", reader.Index());
        }
        if (*name == "fit.start") {
            const auto pointCount = reader.ReadInt();
            if (!pointCount || *pointCount < 2 || *pointCount > 4096) {
                return Invalid("invalid_fit_start", reader.Index());
            }
            std::vector<double> curveDb;
            curveDb.reserve(static_cast<std::size_t>(*pointCount));
            for (long index = 0; index < *pointCount; ++index) {
                const auto value = reader.ReadDouble();
                if (!value) return Invalid("invalid_fit_start", reader.Index());
                curveDb.push_back(*value);
            }
            return reader.RequireEnd()
                ? Success(domain::StartFitCommand{ { *requestId }, std::move(curveDb) })
                : Invalid("invalid_fit_start", reader.Index());
        }
        if (*name == "fit.cancel") {
            const auto sessionId = reader.ReadInt();
            if (!sessionId || *sessionId < 1 || !reader.RequireEnd()) return Invalid("invalid_fit_cancel", reader.Index());
            return Success(domain::CancelFitCommand{ { *requestId }, { *sessionId } });
        }
        if (*name == "fit.clear") return reader.RequireEnd()
            ? Success(domain::ClearFitCommand{ { *requestId } })
            : Invalid("invalid_fit_clear", reader.Index());
        if (*name == "fit.complete") {
            const auto sessionId = reader.ReadInt();
            const auto bankId = reader.ReadInt();
            const auto loss = reader.ReadDouble();
            const auto filterCount = reader.ReadInt();
            if (!sessionId || !bankId || !loss || !filterCount || *sessionId < 1 || *bankId < 1 || *filterCount < 1) {
                return Invalid("invalid_fit_complete", reader.Index());
            }
            domain::FitResult result;
            result.sessionId = { *sessionId };
            result.bankId = { *bankId };
            result.loss = *loss;
            for (long filterIndex = 0; filterIndex < *filterCount; ++filterIndex) {
                const auto filterId = reader.ReadInt();
                const auto bypass = reader.ReadBool();
                const auto valueCount = reader.ReadInt();
                if (!filterId || !bypass || !valueCount || *filterId < 1 ||
                    *filterId > std::numeric_limits<long>::max() || *valueCount < 0) {
                    return Invalid("invalid_fit_complete", reader.Index());
                }
                domain::FilterState filter;
                filter.filterId = static_cast<long>(*filterId);
                filter.bypass = *bypass;
                for (long valueIndex = 0; valueIndex < *valueCount; ++valueIndex) {
                    const auto value = reader.ReadDouble();
                    if (!value) return Invalid("invalid_fit_complete", reader.Index());
                    filter.values.push_back(*value);
                }
                result.filters.push_back(std::move(filter));
            }
            const auto inputGain = reader.ReadDouble();
            const auto compressorBypass = reader.ReadBool();
            const auto attack = reader.ReadDouble();
            const auto release = reader.ReadDouble();
            const auto threshold = reader.ReadDouble();
            const auto output = reader.ReadDouble();
            const auto saturatorBypass = reader.ReadBool();
            const auto saturation = reader.ReadDouble();
            const auto saturatorOutput = reader.ReadDouble();
            const auto outputGain = reader.ReadDouble();
            if (!inputGain || !compressorBypass || !attack || !release || !threshold || !output ||
                !saturatorBypass || !saturation || !saturatorOutput || !outputGain) {
                return Invalid("invalid_fit_complete", reader.Index());
            }
            if (*inputGain < settings::GainOptions::MinimumGainDb ||
                *inputGain > settings::GainOptions::MaximumGainDb ||
                *outputGain < settings::GainOptions::MinimumGainDb ||
                *outputGain > settings::GainOptions::MaximumGainDb ||
                *attack < settings::CompressorOptions::MinimumAttackMs ||
                *attack > settings::CompressorOptions::MaximumAttackMs ||
                *release < settings::CompressorOptions::MinimumReleaseMs ||
                *release > settings::CompressorOptions::MaximumReleaseMs ||
                *threshold < settings::CompressorOptions::MinimumThresholdDb ||
                *threshold > settings::CompressorOptions::MaximumThresholdDb ||
                *output < settings::CompressorOptions::MinimumOutputDb ||
                *output > settings::CompressorOptions::MaximumOutputDb ||
                *saturation < settings::SaturatorOptions::MinimumSaturation ||
                *saturation > settings::SaturatorOptions::MaximumSaturation ||
                *saturatorOutput < settings::SaturatorOptions::MinimumOutputDb ||
                *saturatorOutput > settings::SaturatorOptions::MaximumOutputDb) {
                return Invalid("invalid_fit_complete", reader.Index());
            }
            result.processor.inputGain = { *inputGain };
            result.processor.compressor.attackMs = *attack;
            result.processor.compressor.releaseMs = *release;
            result.processor.compressor.thresholdDb = *threshold;
            result.processor.compressor.outputDb = *output;
            result.processor.compressor.bypass = *compressorBypass;
            result.processor.saturator.saturation = *saturation;
            result.processor.saturator.outputDb = *saturatorOutput;
            result.processor.saturator.bypass = *saturatorBypass;
            result.processor.outputGain = { *outputGain };
            if (!reader.RequireEnd()) return Invalid("invalid_fit_complete", reader.Index());
            return Success(domain::CompleteFitCommand{ { *requestId }, std::move(result) });
        }
        if (*name == "fit.fail") {
            const auto sessionId = reader.ReadInt();
            const auto error = reader.ReadString();
            if (!sessionId || !error || *sessionId < 1 || error->empty() || !reader.RequireEnd()) {
                return Invalid("invalid_fit_fail", reader.Index());
            }
            return Success(domain::FailFitCommand{ { *requestId }, { *sessionId }, *error });
        }

        return Invalid("unknown_command", reader.Index());
    }

    static AtomList EncodeHeader(std::string name, std::string source, long requestId) {
        return { "command", static_cast<std::int64_t>(1), std::move(source),
            static_cast<std::int64_t>(requestId), std::move(name) };
    }

private:
    static DecodedCommand Invalid(std::string code, std::size_t index) {
        return { domain::StartFitCommand{}, { std::move(code), index, "valid command", "invalid" } };
    }

    template <typename Command>
    static DecodedCommand Success(Command command) {
        return { std::move(command), {} };
    }
};

} // namespace consolidator::messaging
