const { ConsolidatorClient } = require("./Clients/ConsolidatorClient.js");
const { PresentationObservable } = require("./Presenters/Core/PresentationObservable.js");
const { ConsolidatorViewModel } = require("./ViewModels/ConsolidatorViewModel.js");
const { BankManagerViewModel } = require("./ViewModels/BankManagerViewModel.js");
const { BankManagerPresenter } = require("./Presenters/BankManager/BankManagerPresenter.js");
const { EqualizerController } = require("./Controllers/EqualizerController.js");
const { CompressorController } = require("./Controllers/CompressorController.js");
const { SaturatorController } = require("./Controllers/SaturatorController.js");
const { GainController } = require("./Controllers/GainController.js");
const { BankManagerController } = require("./Controllers/BankManagerController.js");
const { BankManagerContext } = require("./Controllers/BankManagerContext.js");
const { ControlBindings } = require("./Bindings/ControlBindings.js");
const { AnalyzerControlBinding } = require("./Bindings/AnalyzerControlBinding.js");
const { BankManagerControlBinding } = require("./Bindings/BankManagerControlBinding.js");
const { DialControlBinding } = require("./Bindings/DialControlBinding.js");
const { ButtonControlBinding } = require("./Bindings/ButtonControlBinding.js");

const ConsolidatorControlMapping = {
    inputGain: "input_gain",
    outputGain: "output_gain",
    equalizerAnalyzer: "equalizer_analyzer",
    equalizerBypass: "equalizer_bypass",
    equalizerSolo: "equalizer_solo",
    compressorDetector: "compressor_detector",
    compressorThreshold: "compressor_threshold",
    compressorRatio: "compressor_ratio",
    compressorAttack: "compressor_attack",
    compressorRelease: "compressor_release",
    compressorGain: "compressor_gain",
    compressorMix: "compressor_mix",
    compressorBypass: "compressor_bypass",
    compressorSolo: "compressor_solo",
    saturatorDetector: "saturator_detector",
    saturatorDrive: "saturator_drive",
    saturatorGain: "saturator_gain",
    saturatorMix: "saturator_mix",
    saturatorDetectorAmount: "saturator_detector_amount",
    saturatorBypass: "saturator_bypass",
    saturatorSolo: "saturator_solo",
    bankManager: "bank_manager"
};

class ConsolidatorUiHost
{
    constructor(source, sendNative, sendUi)
    {
        this.source = source;
        this.trackName = null;
        this.sendNative = sendNative || (() => {});
        this.client = new ConsolidatorClient(source, this.sendNative);
        this.sendUi = sendUi || (() => {});
        this.viewModel = new ConsolidatorViewModel(this.client.uiTarget);
        this.equalizer = new EqualizerController(this.viewModel, this.client.scope);
        this.compressor = new CompressorController(this.viewModel, this.client.scope);
        this.saturator = new SaturatorController(this.viewModel, this.client.scope);
        this.equalizer.analyzer.presenter.connectSpectrum(this.client.protocol);
        this.compressor.analyzer.presenter.connectSpectrum(this.client.protocol);
        this.saturator.analyzer.presenter.connectSpectrum(this.client.protocol);
        this.equalizer.analyzer.presenter.connectConfiguration(this.client.protocol);
        this.compressor.analyzer.presenter.connectConfiguration(this.client.protocol);
        this.saturator.analyzer.presenter.connectConfiguration(this.client.protocol);
        this.inputGain = new GainController(this.viewModel.inputGain, this.client.scope);
        this.outputGain = new GainController(this.viewModel.outputGain, this.client.scope);
        this.bankManagerViewModel = new BankManagerViewModel(
            this.client.registry,
            source,
            this.client.transactions,
            this.client.scope
        );
        this.bankManagerPresenter = new BankManagerPresenter(
            this.bankManagerViewModel
        );
        this.bankManager = new BankManagerController(
            new BankManagerContext(
                this.bankManagerViewModel,
                this.client.state,
                this.client.uiTarget,
                source,
                this.client.protocol,
                this.client.transactions,
                undefined,
                this.client.scope
            )
        );
        this.bindings = new ControlBindings();
        this.client.targetState.onTargetSnapshotBatchBegin(() => {
            PresentationObservable.beginBatch();
        });
        this.client.targetState.onTargetSnapshotBatchEnd(() => {
            PresentationObservable.endBatch();
        });
        this.client.targetState.onTargetTransitionBegin(() => {
            this.bindings.suspend();
        });
        this.client.targetState.onTargetTransitionDone(() => {
            this.bindings.resumeLatest();
        });
        this.registryInitialized = false;
        this.lifecycle = "created";
        this.instanceActive = false;
        this.publishedInstanceActive = null;
        this.metricsGestureActive = false;
        this.snapshotContext = "equalizer";
    }
    
    sendControlMessage(
        controlName,
        selector,
        args
    )
    {
        this.sendUi([controlName, selector].concat(args || []));
    }
    
    handleControl(selector, args)
    {
        if (selector === "instance_active") {
            if (args && args.length > 0) {
                this.setInstanceActive(Number(args[0]) !== 0);
            }
            return;
        }
        this.client.handleControl(selector, args);
    }
    
    setInstanceActive(active)
    {
        this.instanceActive = Boolean(active);
        if (this.instanceId !== undefined &&
                this.publishedInstanceActive === this.instanceActive) {
            return;
        }
        this.bankManagerViewModel.setRegistryActive(this.instanceActive);
        this.bindings.setPresentationActive(false);
        if (this.instanceId === undefined) {
            return;
        }
        this.publishedInstanceActive = this.instanceActive;
        this.client.setInstanceActive(this.instanceActive, (response) => {
            if (!response || response.error) {
                this.publishedInstanceActive = null;
                return;
            }
            if (!this.instanceActive) {
                return;
            }
            const target = this.client.targetState.target || {
                instanceId: this.instanceId,
                bankId: 0
            };
            this.client.uiTarget.show(
                target.instanceId,
                target.bankId,
                this.snapshotContext,
                (snapshotResponse) => {
                    if (this.instanceActive && snapshotResponse &&
                            !snapshotResponse.error) {
                        this.bindings.setPresentationActive(true);
                    }
                }
            );
        });
    }
    
    setTrackName(args)
    {
        args = args || [];
        if (args.length && String(args[0]) === "name") {
            args = args.slice(1);
        }
        let name = args.join(" ");
        if (name.length >= 2 && name.charAt(0) === '"' &&
                name.charAt(name.length - 1) === '"') {
            name = name.substring(1, name.length - 1);
        }
        this.trackName = name;
        if (this.instanceId !== undefined) {
            this.client.state.set("label", name, undefined, 0, "local");
        }
    }
    
    bind(varname, binding)
    {
        this.bindings.add(varname, binding((selector, args) => {
            this.sendControlMessage(varname, selector, args);
        }));
    }
    
    handleUiIntent(
        controlName,
        intent,
        values
    )
    {
        if (intent === "gestureBegan") {
            this.metricsGestureActive = true;
            this.sendMetrics();
        }
        this.bindings.handle(controlName, intent, values);
        if (intent === "gestureEnded") {
            this.metricsGestureActive = false;
            this.sendMetrics();
        }
        else if (intent !== "gestureBegan" && !this.metricsGestureActive) {
            this.sendMetrics();
        }
    }
    
    sendMetrics()
    {
        this.sendNative(["metrics"]);
    }
    
    bindControls()
    {
        const mapping = this.mapping;
        this.bind(mapping.equalizerAnalyzer, (send) => {
            return new AnalyzerControlBinding(
                this.equalizer.analyzer,
                this.equalizer.analyzer.presenter,
                send,
                this.client.transactions
            );
        });
        this.bind(mapping.compressorDetector, (send) => {
            return new AnalyzerControlBinding(
                this.compressor.analyzer,
                this.compressor.analyzer.presenter,
                send,
                this.client.transactions
            );
        });
        this.bind(mapping.saturatorDetector, (send) => {
            return new AnalyzerControlBinding(
                this.saturator.analyzer,
                this.saturator.analyzer.presenter,
                send,
                this.client.transactions
            );
        });
        this.bind(mapping.bankManager, (send) => {
            return new BankManagerControlBinding(
                this.bankManager,
                this.bankManagerPresenter,
                send
            );
        });
        [
            ["input", this.inputGain],
            ["equalizer", this.equalizer],
            ["compressor", this.compressor],
            ["saturator", this.saturator],
            ["output", this.outputGain]
        ].forEach(([prefix, controller]) => {
            controller.presenters.forEach((name, presenter, type) => {
                const controlName = prefix + name.charAt(0).toUpperCase() +
                    name.substring(1);
                this.bind(mapping[controlName], (send) => {
                    return type === "dial"
                        ? new DialControlBinding(
                            presenter,
                            send,
                            this.client.transactions)
                        : new ButtonControlBinding(presenter, send);
                });
            });
        });
    }
    
    initialize(mapping, callback)
    {
        if (this.lifecycle !== "created") {
            return;
        }
        this.lifecycle = "initializing";
        this.mapping = mapping || ConsolidatorControlMapping;
        this.bindControls();
        this.client.initialize((initialization) => {
            if (initialization.error) {
                this.lifecycle = "initialized";
                this.registryInitialized = false;
                if (callback) callback(initialization.error);
                return;
            }
            this.instanceId = initialization.instanceId;
            this.snapshotContext = initialization.snapshotContext || "equalizer";
            this.bankManagerViewModel.setSelectedPanel(this.snapshotContext);
            this.viewModel.instanceId = this.instanceId;
            this.bankManager.context.instanceId = this.instanceId;
            this.bankManagerViewModel.setLocalInstanceId(this.instanceId);
            this.bankManagerViewModel.setFocusedBank(this.instanceId, 0);
            if (this.trackName !== null) {
                this.client.state.set(
                    "label",
                    this.trackName,
                    undefined,
                    0,
                    "local"
                );
            }
            const finishInitialization = () => {
                this.setInstanceActive(this.instanceActive);
                this.registryInitialized = true;
                this.lifecycle = "initialized";
                if (callback) callback(null);
            };
            if (this.instanceActive) {
                this.bankManagerViewModel.setRegistryActive(true, (snapshot, response) => {
                    if (response && response.error) {
                        this.registryInitialized = false;
                        this.lifecycle = "initialized";
                        if (callback) callback(response.error);
                        return;
                    }
                    finishInitialization();
                });
            }
            else {
                finishInitialization();
            }
            this.viewModel.initialize((error) => {
                if (error && callback) callback(error);
            });
        });
    }
    
    undo(callback)
    {
        return this.client.transactions.undo(callback);
    }
    
    redo(callback)
    {
        return this.client.transactions.redo(callback);
    }
    
    destroy()
    {
        if (this.lifecycle === "destroyed") {
            return;
        }
        this.lifecycle = "destroyed";
        if (this.instanceId !== undefined && this.publishedInstanceActive) {
            this.client.setInstanceActive(false);
        }
        this.bindings.destroy();
        this.equalizer.destroy();
        this.compressor.destroy();
        this.saturator.destroy();
        this.inputGain.destroy();
        this.outputGain.destroy();
        this.bankManager.destroy();
        this.bankManagerPresenter.destroy();
        this.bankManagerViewModel.destroy();
        this.viewModel.destroy();
        this.client.destroy();
        this.metricsGestureActive = false;
        this.sendNative = () => {};
    }
}

module.exports = {
    ConsolidatorUiHost: ConsolidatorUiHost,
    ConsolidatorControlMapping: ConsolidatorControlMapping
};
