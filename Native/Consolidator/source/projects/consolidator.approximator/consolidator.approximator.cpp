#include "c74_min.h"

#include "ApproximatorCurveStore.h"
#include "ApproximatorOutputs.h"
#include "EqOptimizer.h"
#include "MaxFilterDefinitionAdapter.h"
#include "MaxMessageAdapter.h"
#include "Messaging/MessageRegistry.h"
#include "Messaging/Messages/ApproximatorClearMessage.h"
#include "Messaging/Messages/ApproximatorFitMessage.h"
#include "Messaging/Messages/EqBankChangedMessage.h"
#include "Messaging/Messages/FilterDefinitionMessage.h"

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

using namespace c74::min;

class ConsolidatorApproximator : public object<ConsolidatorApproximator> {
public:
    MIN_DESCRIPTION{ "Consolidator EQ curve approximator." };
    MIN_TAGS{ "audio, eq, optimizer" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> commands{
        this,
        "(message) commands: message <dictionary type=filter.define|eq.storage.bank.changed|approximator.clear|approximator.fit>"
    };
    inlet<> inputCurve{ this, "(list) target difference curve in dB" };
    inlet<> currentEqCurve{ this, "(list) current summed EQ curve in dB" };

    outlet<> commandsOut{
        this,
        "(message) commands: message <dictionary type=filter.apply payload=filterId,values,bankIndex>"
    };
    outlet<> statusOut{ this, "(anything) status: ready 0/1" };
    outlet<> debugOut{ this, "(anything) diagnostics: fit_started, fit_finished, error <code>, loss <value>" };

    queue<> fitDelivery{
        this,
        MIN_FUNCTION {
            DeliverFitResult();
            return {};
        }
    };

    message<> list{
        this,
        "list",
        "Receive analysis curves",
        MIN_FUNCTION {
            if (inlet == 1) curveStore.SetTarget(args);
            else if (inlet == 2) curveStore.SetCurrentEq(args);
            UpdateReady();
            return {};
        }
    };

    message<> loadbang{
        this,
        "loadbang",
        MIN_FUNCTION {
            SetReady(false, true);
            return {};
        }
    };

    message<> envelopeMessage{
        this,
        "message",
        "Receive a structured approximator control envelope",
        MIN_FUNCTION {
            if (inlet != 0 || args.size() != 1) {
                debugOut.send("error", "invalid_message_envelope");
                return {};
            }
            const auto envelope = consolidator::maxadapter::MaxMessageAdapter::Deserialize(args[0]);
            if (!envelope) {
                debugOut.send("error", "invalid_message_envelope");
                return {};
            }
            if (!consolidator::maxadapter::MaxMessageAdapter::IsAddressedTo(*envelope, "approximator")) {
                return {};
            }
            const auto command = messageFactory.Deserialize(*envelope);
            if (const auto* definition = dynamic_cast<consolidator::messaging::FilterDefinitionMessage*>(command.get())) {
                Handle(*definition);
            }
            else if (const auto* bank = dynamic_cast<consolidator::messaging::EqBankChangedMessage*>(command.get())) {
                Handle(*bank);
            }
            else if (dynamic_cast<consolidator::messaging::ApproximatorClearMessage*>(command.get())) {
                curveStore.ClearTarget();
                UpdateReady();
            }
            else if (dynamic_cast<consolidator::messaging::ApproximatorFitMessage*>(command.get())) {
                StartFit();
            }
            else {
                debugOut.send("error", "invalid_message_envelope");
            }
            return {};
        }
    };

    ~ConsolidatorApproximator() override {
        if (fitWorker.joinable()) fitWorker.join();
    }

private:
    using Definitions = EqOptimizer::Definitions;

    void Handle(const consolidator::messaging::FilterDefinitionMessage& command) {
        const auto definition = consolidator::maxadapter::MaxFilterDefinitionAdapter::Read(
            command.contractName, command.filterId, command.defaultBypass);
        if (!definition) {
            debugOut.send("error", "invalid_filter_definition");
            return;
        }
        definitions[definition->filterId] = *definition;
        UpdateReady();
    }

    void Handle(const consolidator::messaging::EqBankChangedMessage& command) {
        fitBankIndex = command.bankIndex;
        curveStore.ClearTarget();
        UpdateReady();
    }

    void StartFit() {
        ApproximatorOutputs outputs{ commandsOut, statusOut, debugOut };
        if (fitBankIndex < 1) {
            outputs.Error("no_selected_bank");
            SetReady(false);
            return;
        }
        if (definitions.empty()) {
            outputs.Error("no_defined_filters");
            SetReady(false);
            return;
        }
        if (!curveStore.HasTarget()) {
            outputs.Error("no_difference_curve");
            SetReady(false);
            return;
        }
        if (!curveStore.HasCurrentEq()) {
            outputs.Error("no_current_eq_curve");
            SetReady(false);
            return;
        }
        if (!curveStore.HasCompatibleCurves()) {
            outputs.Error("curve_size_mismatch");
            SetReady(false);
            return;
        }

        const auto curve = curveStore.CombinedCurve();
        const auto fitDefinitions = definitions;
        {
            std::lock_guard<std::mutex> lock(fitMutex);
            if (fitRunning) {
                outputs.Error("fit_in_progress");
                return;
            }
            fitRunning = true;
            activeFitBankIndex = fitBankIndex;
            pendingError.clear();
            pendingResult.reset();
        }
        if (fitWorker.joinable()) fitWorker.join();
        SetReady(false);
        outputs.FitStarted();
        fitWorker = std::thread([this, curve, fitDefinitions]() {
            try {
                const auto result = optimizer.Fit(curve, fitDefinitions);
                std::lock_guard<std::mutex> lock(fitMutex);
                pendingResult = result;
            }
            catch (const std::exception& error) {
                std::lock_guard<std::mutex> lock(fitMutex);
                pendingError = error.what();
            }
            fitDelivery.set();
        });
    }

    void DeliverFitResult() {
        std::optional<EqOptimizer::FitResult> result;
        std::string error;
        {
            std::lock_guard<std::mutex> lock(fitMutex);
            result = pendingResult;
            error = pendingError;
            pendingResult.reset();
            pendingError.clear();
        }
        if (fitWorker.joinable()) fitWorker.join();
        fitRunning = false;

        ApproximatorOutputs outputs{ commandsOut, statusOut, debugOut };
        if (!error.empty()) outputs.Error(error.c_str());
        else if (result) {
            outputs.Loss(result->loss);
            outputs.SendFilterCommands(definitions, result->solverValues, activeFitBankIndex);
        }
        outputs.FitFinished();
        UpdateReady();
    }

    void UpdateReady() {
        SetReady(
            fitBankIndex >= 1 && curveStore.HasCompatibleCurves() && !definitions.empty() && !fitRunning.load()
        );
    }

    void SetReady(bool available, bool force = false) {
        if (!force && fitRunning) available = false;
        if (!force && readyAvailable == available) return;
        readyAvailable = available;
        ApproximatorOutputs{ commandsOut, statusOut, debugOut }.Ready(available);
    }

    ApproximatorCurveStore curveStore;
    Definitions definitions;
    EqOptimizer optimizer;
    consolidator::messaging::MessageFactory messageFactory =
        consolidator::messaging::MessageRegistry::CreateFactory();
    std::mutex fitMutex;
    std::thread fitWorker;
    std::atomic<bool> fitRunning = false;
    std::optional<EqOptimizer::FitResult> pendingResult;
    std::string pendingError;
    bool readyAvailable = false;
    long fitBankIndex = 0;
    long activeFitBankIndex = 0;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorApproximator, consolidator.approximator);
