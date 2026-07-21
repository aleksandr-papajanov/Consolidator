#pragma once

#include "AtomReader.h"
#include "AtomWriter.h"
#include "Commands/Commands.h"

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
        if (*name == "eq.set_section_bypass") {
            const auto bankId = reader.ReadInt();
            const auto sectionName = reader.ReadString();
            const auto bypass = reader.ReadBool();
            if (!bankId || !sectionName || !bypass || *bankId < 1 || !reader.RequireEnd()) return Invalid("invalid_eq_set_section_bypass", reader.Index());
            const auto section = *sectionName == "pre" ? models::EqSection::Pre
                : *sectionName == "post" ? models::EqSection::Post
                : models::EqSection::Pre;
            if (*sectionName != "pre" && *sectionName != "post") return Invalid("invalid_eq_section", reader.Index());
            return Success(domain::SetEqSectionBypassCommand{ { *requestId }, { *bankId }, section, *bypass });
        }
        if (*name == "eq.reset_section") {
            const auto bankId = reader.ReadInt();
            const auto sectionName = reader.ReadString();
            if (!bankId || !sectionName || *bankId < 1 || !reader.RequireEnd()) return Invalid("invalid_eq_reset_section", reader.Index());
            const auto section = *sectionName == "pre" ? models::EqSection::Pre
                : *sectionName == "post" ? models::EqSection::Post
                : models::EqSection::Pre;
            if (*sectionName != "pre" && *sectionName != "post") return Invalid("invalid_eq_section", reader.Index());
            return Success(domain::ResetEqSectionCommand{ { *requestId }, { *bankId }, section });
        }
        if (*name == "eq.add_bank") {
            if (reader.RequireEnd()) {
                return Success(domain::AddEqBankCommand{ { *requestId }, {} });
            }
            const auto bankName = reader.ReadString();
            if (!bankName || !reader.RequireEnd()) return Invalid("invalid_eq_add_bank", reader.Index());
            return Success(domain::AddEqBankCommand{ { *requestId }, *bankName });
        }
        if (*name == "eq.remove_bank") {
            const auto bankId = reader.ReadInt();
            if (!bankId || *bankId < 1 || !reader.RequireEnd()) return Invalid("invalid_eq_remove_bank", reader.Index());
            return Success(domain::RemoveEqBankCommand{ { *requestId }, { *bankId } });
        }
        if (*name == "eq.rename_bank") {
            const auto bankId = reader.ReadInt();
            const auto bankName = reader.ReadString();
            if (!bankId || !bankName || *bankId < 1 || bankName->empty() || !reader.RequireEnd()) return Invalid("invalid_eq_rename_bank", reader.Index());
            return Success(domain::RenameEqBankCommand{ { *requestId }, { *bankId }, *bankName });
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
        if (*name == "compressor.reset") return reader.RequireEnd()
            ? Success(domain::ResetCompressorCommand{ { *requestId } })
            : Invalid("invalid_compressor_reset", reader.Index());
        if (*name == "saturator.set_parameter") {
            const auto value = reader.ReadDouble();
            if (!value || !reader.RequireEnd()) return Invalid("invalid_saturator_set_parameter", reader.Index());
            return Success(domain::SetSaturatorParameterCommand{ { *requestId }, *value });
        }
        if (*name == "saturator.set_bypass") {
            const auto bypass = reader.ReadBool();
            if (!bypass || !reader.RequireEnd()) return Invalid("invalid_saturator_set_bypass", reader.Index());
            return Success(domain::SetSaturatorBypassCommand{ { *requestId }, *bypass });
        }
        if (*name == "saturator.reset") return reader.RequireEnd()
            ? Success(domain::ResetSaturatorCommand{ { *requestId } })
            : Invalid("invalid_saturator_reset", reader.Index());
        if (*name == "analyzer.listen") {
            const auto enabled = reader.ReadBool();
            if (!enabled || !reader.RequireEnd()) return Invalid("invalid_analyzer_listen", reader.Index());
            return Success(domain::ListenAnalyzerCommand{ { *requestId }, *enabled });
        }
        if (*name == "fit.start") return reader.RequireEnd()
            ? Success(domain::StartFitCommand{ { *requestId } })
            : Invalid("invalid_fit_start", reader.Index());
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
