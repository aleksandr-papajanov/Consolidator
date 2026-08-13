include("Clients/NativeProtocolClient.js");
include("Clients/StateClient.js");
include("Clients/AnalysisClient.js");
include("Clients/RegistryClient.js");
include("Clients/ConsolidatorClient.js");
include("ViewModels/ObservableValue.js");
include("ViewModels/StateValueViewModel.js");
include("ViewModels/FilterViewModel.js");
include("ViewModels/DetectorFilterViewModel.js");
include("ViewModels/BankViewModel.js");
include("ViewModels/GainViewModel.js");
include("ViewModels/CompressorViewModel.js");
include("ViewModels/SaturatorViewModel.js");
include("ViewModels/EqualizerViewModel.js");
include("ViewModels/AnalyzerViewModel.js");
include("ViewModels/ConsolidatorViewModel.js");
include("ViewModels/BankManagerViewModel.js");
include("Presenters/Analyzer/AnalyzerPresenter.js");
include("Presenters/BankManager/BankManagerPresenter.js");
include("Controllers/EqualizerController.js");
include("Controllers/CompressorController.js");
include("Controllers/SaturatorController.js");
include("Controllers/GainController.js");
include("Controllers/BankManagerController.js");
include("Bindings/ControlBindings.js");

function ConsolidatorUiHost(source, sendNative, sendUi) {
    this.source = source;
    this.client = new ConsolidatorClient(source, sendNative);
    this.sendUi = sendUi || function () {};
    this.viewModel = new ConsolidatorViewModel(this.client);
    this.equalizer = new EqualizerController(this.viewModel);
    this.compressor = new CompressorController(this.viewModel);
    this.saturator = new SaturatorController(this.viewModel);
    this.inputGain = new GainController(this.viewModel.inputGain);
    this.outputGain = new GainController(this.viewModel.outputGain);
    this.bankManagerViewModel = new BankManagerViewModel(
        this.client.registry,
        source
    );
    this.bankManagerPresenter = new BankManagerPresenter(
        this.bankManagerViewModel
    );
    this.bankManager = new BankManagerController(
        new BankManagerContext(
            this.bankManagerViewModel,
            this.client.state,
            this.viewModel.analyzer,
            this.viewModel.selectedBank,
            source
        )
    );
    this.bindings = new ControlBindings();
    this.initialized = false;
    this.lifecycle = "created";
}

var ConsolidatorControlMapping = {
    bankManager: "bank_manager",
    inputGain: "input_gain",
    outputGain: "output_gain",
    equalizerAnalyzer: "equalizer_analyzer",
    compressorDetector: "compressor_detector",
    saturatorDetector: "saturator_detector",
    equalizerBypass: "equalizer_bypass",
    equalizerSolo: "equalizer_solo",
    compressorThreshold: "compressor_threshold",
    compressorRatio: "compressor_ratio",
    compressorAttack: "compressor_attack",
    compressorRelease: "compressor_release",
    compressorGain: "compressor_gain",
    compressorMix: "compressor_mix",
    compressorBypass: "compressor_bypass",
    compressorSolo: "compressor_solo",
    saturatorDrive: "saturator_drive",
    saturatorGain: "saturator_gain",
    saturatorMix: "saturator_mix",
    saturatorDetectorAmount: "saturator_detector_amount",
    saturatorBypass: "saturator_bypass",
    saturatorSolo: "saturator_solo"
};

ConsolidatorUiHost.prototype.sendControlMessage = function (
    controlName,
    selector,
    args
) {
    this.sendUi([controlName, selector].concat(args || []));
};

ConsolidatorUiHost.prototype.handleControl = function (selector, args) {
    this.client.handleControl(selector, args);
};

ConsolidatorUiHost.prototype.handleAnalysis = function (selector, args) {
    this.client.handleAnalysis(selector, args);
};

ConsolidatorUiHost.prototype.tickAnalysis = function () {
    this.client.analysis.tick();
};

ConsolidatorUiHost.prototype.bind = function (varname, binding) {
    var self = this;
    this.bindings.add(varname, binding(function (selector, args) {
        self.sendControlMessage(varname, selector, args);
    }));
};

ConsolidatorUiHost.prototype.handleUiIntent = function (
    controlName,
    intent,
    values
) {
    this.bindings.handle(controlName, intent, values);
};

ConsolidatorUiHost.prototype.bindControls = function () {
    var self = this;
    var mapping = this.mapping;
    this.bind(mapping.equalizerAnalyzer, function (send) {
        return new AnalyzerControlBinding(
            self.equalizer.analyzer,
            self.equalizer.analyzer.presenter,
            send
        );
    });
    this.bind(mapping.compressorDetector, function (send) {
        return new AnalyzerControlBinding(
            self.compressor.analyzer,
            self.compressor.analyzer.presenter,
            send
        );
    });
    this.bind(mapping.saturatorDetector, function (send) {
        return new AnalyzerControlBinding(
            self.saturator.analyzer,
            self.saturator.analyzer.presenter,
            send
        );
    });
    this.bind(mapping.bankManager, function (send) {
        return new BankManagerControlBinding(
            self.bankManager,
            self.bankManagerPresenter,
            send
        );
    });
    [
        ["input", self.inputGain],
        ["equalizer", self.equalizer],
        ["compressor", self.compressor],
        ["saturator", self.saturator],
        ["output", self.outputGain]
    ].forEach(function (feature) {
        var prefix = feature[0];
        var controller = feature[1];
        controller.presenters.forEach(function (name, presenter, type) {
            var controlName = prefix + name.charAt(0).toUpperCase() +
                name.substring(1);
            self.bind(mapping[controlName], function (send) {
                return type === "dial"
                    ? new DialControlBinding(presenter, send)
                    : new ButtonControlBinding(presenter, send);
            });
        });
    });
};

ConsolidatorUiHost.prototype.initialize = function (mapping, callback) {
    if (this.lifecycle !== "created") {
        return;
    }
    this.lifecycle = "initializing";
    var self = this;
    this.mapping = mapping || ConsolidatorControlMapping;
    this.bindControls();
    this.client.registry.fetch();
    this.viewModel.initialize(function (error) {
        if (self.lifecycle === "destroyed") {
            return;
        }
        if (self.viewModel.selectedBank.instanceId !== undefined) {
            self.viewModel.instanceId = self.viewModel.selectedBank.instanceId;
            self.bankManager.context.instanceId =
                self.viewModel.selectedBank.instanceId;
            self.bankManagerViewModel.setLocalInstanceId(
                self.viewModel.selectedBank.instanceId
            );
        }
        var selectedBank = self.viewModel.selectedBank.value;
        if (selectedBank !== undefined && selectedBank !== null) {
            self.viewModel.analyzer.show(
                self.viewModel.selectedBank.instanceId,
                selectedBank
            );
        }
        self.initialized = !error;
        self.lifecycle = "initialized";
        if (callback) {
            callback(error || null);
        }
    });
};

ConsolidatorUiHost.prototype.destroy = function () {
    if (this.lifecycle === "destroyed") {
        return;
    }
    this.lifecycle = "destroyed";
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
};

inlets = 3;
outlets = 2;

function sendNative(frame) {
    outlet(0, frame);
}

function sendUi(frame) {
    outlet(1, frame);
}

function ensureHost() {
    if (!uiHost) {
        var source = jsarguments.length > 1
            ? String(jsarguments[1]) : "consolidator.ui";
        uiHost = new ConsolidatorUiHost(source, sendNative, sendUi);
    }
    return uiHost;
}

function loadbang() {
    ensureHost().initialize(ConsolidatorControlMapping);
}

function anything() {
    var args = arrayfromargs(arguments);
    if (inlet === 0) {
        ensureHost().handleControl(messagename, args);
    } else if (inlet === 1) {
        ensureHost().handleAnalysis(messagename, args);
    } else {
        var intent = args.shift();
        ensureHost().handleUiIntent(messagename, intent, args);
    }
}

function analysis_tick() {
    ensureHost().tickAnalysis();
}

function destroy() {
    if (uiHost) {
        uiHost.destroy();
        uiHost = null;
    }
}

function notifydeleted() {
    destroy();
}

var uiHost = null;
