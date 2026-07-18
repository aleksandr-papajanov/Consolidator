#include "c74_min.h"

#include "FilterContract.h"
#include "FilterContractDictionary.h"
#include "FilterChain.h"
#include "MessageFactory.h"
#include "TypedMessages.h"

#include <array>
#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace c74::min;

class ConsolidatorEqChain :
    public object<ConsolidatorEqChain>,
    public sample_operator<2, 2> {
public:
    MIN_DESCRIPTION{ "Consolidator EQ chain audio processor." };
    MIN_TAGS{ "audio, eq, chain" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> input_l{ this, "(signal) left input", "signal" };
    inlet<> input_r{ this, "(signal) right input", "signal" };
    inlet<> commands_in{
        this,
        "(message) commands: message <dictionary type=filter.define|eq.storage.snapshot>"
    };

    outlet<> output_l{ this, "(signal) left output", "signal" };
    outlet<> output_r{ this, "(signal) right output", "signal" };
    outlet<> debug_out{ this, "(anything) diagnostics: error <code>" };

    message<> dspsetup{
        this,
        "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                sample_rate_ = static_cast<double>(args[0]);
                rebuild_bank_chains();
            }

            return {};
        }
    };

    message<> envelope_message{
        this,
        "message",
        "Apply a structured control envelope",
        MIN_FUNCTION {
            if (inlet != 2 || args.size() != 1) {
                debug_out.send("error", "invalid_message_envelope");
                return {};
            }

            auto message = consolidator::protocol::MessageFactory::from_atom(args[0]);
            if (!message) {
                debug_out.send("error", "invalid_message_envelope");
                return {};
            }
            if (!message->is_addressed_to("eq.chain")) {
                return {};
            }
            const auto result = consolidator::protocol::dispatch<
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

    samples<2> operator()(sample in_l, sample in_r) {
        double left = in_l;
        double right = in_r;
        const auto runtime = runtime_state_.load(std::memory_order_acquire);
        for (auto& [bank_id, chain] : runtime->chains) {
            const auto output = chain.process(left, right);
            left = output.first;
            right = output.second;
        }
        return { left, right };
    }

private:
    struct StoredFilter {
        std::vector<double> values;
        bool bypassed = false;
    };

    struct EqBankState {
        std::array<std::optional<StoredFilter>, FilterChain::max_filters> filters{};
    };

    struct RuntimeState {
        std::map<long, FilterChain> chains;
    };

    bool valid_slot(const long filterId) const {
        return filterId >= 0 && static_cast<std::size_t>(filterId) < contracts_.size();
    }

    void handle_command(const consolidator::protocol::FilterDefineMessage& command) {
        if (!valid_slot(command.filterId)) {
            debug_out.send("error", "invalid_filter_slot");
            return;
        }
        FilterContract contract;
        const bool parsed = command.contractName.empty()
            ? parse_filter_contract_dictionary_for_slot(
                contract, command.contract, static_cast<int>(command.filterId))
            : [&]() {
                const dict configuration{ symbol(command.contractName.c_str()) };
                return parse_filter_contract_dictionary_for_slot(
                    contract,
                    atom{ static_cast<c74::max::t_object*>(configuration) },
                    static_cast<int>(command.filterId));
            }();
        if (!parsed) {
            debug_out.send("error", "invalid_filter_definition");
            return;
        }
        contracts_[static_cast<std::size_t>(command.filterId)] = contract;
        rebuild_bank_chains();
    }

    void handle_command(const consolidator::protocol::EqStorageSnapshotMessage& command) {
        std::map<long, EqBankState> next_banks;
        try {
            dict snapshot{ symbol(command.snapshotName.c_str()) };
            dict source_banks{ static_cast<atom>(snapshot.at("banks")) };
            for (const auto& bank_symbol : source_banks.keys()) {
                const auto bank_id = std::stol(static_cast<const char* const>(bank_symbol));
                dict source_bank{ static_cast<atom>(source_banks.at(bank_symbol)) };
                dict source_filters{ static_cast<atom>(source_bank.at("filters")) };
                auto& bank = next_banks[bank_id];
                for (const auto& filter_symbol : source_filters.keys()) {
                    const auto slot = std::stol(static_cast<const char* const>(filter_symbol));
                    if (slot < 0 || static_cast<std::size_t>(slot) >= FilterChain::max_filters) {
                        continue;
                    }
                    dict source_filter{ static_cast<atom>(source_filters.at(filter_symbol)) };
                    const auto values = static_cast<std::vector<number>>(source_filter.at("values"));
                    bool bypassed = false;
                    try {
                        bypassed = static_cast<double>(static_cast<atom>(source_filter.at("bypass"))) != 0.0;
                    }
                    catch (...) {
                    }
                    bank.filters[static_cast<std::size_t>(slot)] = StoredFilter{
                        std::vector<double>(values.begin(), values.end()), bypassed
                    };
                }
            }
        }
        catch (...) {
            debug_out.send("error", "invalid_eq_storage_snapshot");
            return;
        }
        banks_ = std::move(next_banks);
        rebuild_bank_chains();
    }

    void rebuild_bank_chains() {
        auto next_runtime = std::make_shared<RuntimeState>();
        for (const auto& [bankId, bank] : banks_) {
            BuildBankChain(*next_runtime, bankId, bank);
        }
        runtime_state_.store(std::move(next_runtime), std::memory_order_release);
    }

    void BuildBankChain(
        RuntimeState& runtime,
        const long chain_order,
        const EqBankState& bank
    ) {
        auto& chain = runtime.chains[chain_order];
        chain.set_sample_rate(sample_rate_);
        for (std::size_t slot = 0; slot < bank.filters.size(); ++slot) {
            if (!bank.filters[slot] || !contracts_[slot]) {
                continue;
            }
            const auto& filter = *bank.filters[slot];
            const auto& contract = *contracts_[slot];
            if (filter.values.size() != contract_parameter_count(contract)) {
                continue;
            }
            if (!AbsoluteValuesMatchContract(contract, filter.values)) {
                continue;
            }
            chain.set_filter(slot, AbsoluteValuesToSpec(contract, filter.values));
            chain.set_filter_bypass(slot, filter.bypassed);
        }
    }

    double sample_rate_ = EqCurveGrid::default_sample_rate;
    std::map<long, EqBankState> banks_;
    std::atomic<std::shared_ptr<RuntimeState>> runtime_state_{
        std::make_shared<RuntimeState>()
    };
    std::array<std::optional<FilterContract>, FilterChain::max_filters> contracts_{};
};

MIN_EXTERNAL_CUSTOM(ConsolidatorEqChain, consolidator.eqchain);
