include("Project:/js/Clients/NativeProtocolClient.js");
include("Project:/js/Clients/TransactionClient.js");
include("Project:/js/Clients/StateClient.js");
include("Project:/js/Clients/TargetStateClient.js");
include("Project:/js/Clients/UiTarget.js");
include("Project:/js/Clients/RegistryClient.js");
include("Project:/js/Clients/ConsolidatorClient.js");
include("Project:/js/ViewModels/ObservableValue.js");
include("Project:/js/ViewModels/StateValueViewModel.js");
include("Project:/js/ViewModels/FilterViewModel.js");
include("Project:/js/ViewModels/DetectorFilterViewModel.js");
include("Project:/js/ViewModels/GainViewModel.js");
include("Project:/js/ViewModels/CompressorViewModel.js");
include("Project:/js/ViewModels/SaturatorViewModel.js");
include("Project:/js/ViewModels/EqualizerViewModel.js");
include("Project:/js/ViewModels/ConsolidatorViewModel.js");
include("Project:/js/ViewModels/BankManagerViewModel.js");
include("Project:/js/Presenters/BankManager/BankManagerPresenter.js");
include("Project:/js/Controllers/EqualizerController.js");
include("Project:/js/Controllers/CompressorController.js");
include("Project:/js/Controllers/SaturatorController.js");
include("Project:/js/Controllers/GainController.js");
include("Project:/js/Controllers/BankManagerController.js");
include("Project:/js/Bindings/ControlBindings.js");

function ConsolidatorUiHost(source, sendNative, sendUi) {
    this.source = source;
    this.trackName = null;
    this.client = new ConsolidatorClient(source, sendNative);
    this.sendUi = sendUi || function () {};
    this.viewModel = new ConsolidatorViewModel(this.client.uiTarget);
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
            this.client.uiTarget,
            source
        )
    );
    this.bindings = new ControlBindings();
    this.registryInitialized = false;
    this.lifecycle = "created";
}

var ConsolidatorControlMapping = {
    bankManager: "bank_manager",
    equalizerAnalyzer: "equalizer_analyzer",
    compressorDetector: "compressor_detector",
    saturatorDetector: "saturator_detector",
    inputGain: "input_gain",
    outputGain: "output_gain",
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

ConsolidatorUiHost.prototype.setTrackName = function (args) {
    args = args || [];
    if (args.length && String(args[0]) === "name") {
        args = args.slice(1);
    }
    var name = args.join(" ");
    if (name.length >= 2 && name.charAt(0) === '"' &&
            name.charAt(name.length - 1) === '"') {
        name = name.substring(1, name.length - 1);
    }
    this.trackName = name;
    if (this.instanceId !== undefined) {
        this.client.state.setFor(this.instanceId, "label", name);
    }
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
            send,
            self.client.transactions
        );
    });
    this.bind(mapping.compressorDetector, function (send) {
        return new AnalyzerControlBinding(
            self.compressor.analyzer,
            self.compressor.analyzer.presenter,
            send,
            self.client.transactions
        );
    });
    this.bind(mapping.saturatorDetector, function (send) {
        return new AnalyzerControlBinding(
            self.saturator.analyzer,
            self.saturator.analyzer.presenter,
            send,
            self.client.transactions
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
                    ? new DialControlBinding(
                        presenter,
                        send,
                        self.client.transactions)
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
    this.client.initialize(function (initialization) {
        if (initialization.error) {
            self.lifecycle = "initialized";
            self.registryInitialized = false;
            if (callback) callback(initialization.error);
            return;
        }
        self.instanceId = initialization.instanceId;
        self.viewModel.instanceId = self.instanceId;
        self.bankManager.context.instanceId = self.instanceId;
        self.bankManagerViewModel.setLocalInstanceId(self.instanceId);
        if (self.trackName !== null) {
            self.client.state.setFor(
                self.instanceId,
                "label",
                self.trackName
            );
        }
        self.client.registry.fetch(function (snapshot, response) {
            if (response && response.error) {
                self.registryInitialized = false;
                self.lifecycle = "initialized";
                if (callback) callback(response.error);
                return;
            }
            var local = (snapshot.instances || []).filter(function (item) {
                return String(item.instanceId) === String(self.instanceId);
            })[0];
            if (local) {
                self.bankManagerViewModel.setFocusedBank(self.instanceId, 1);
                self.viewModel.show(self.instanceId, 1);
            }
            self.registryInitialized = true;
            self.lifecycle = "initialized";
            if (callback) callback(null);
        });
        self.viewModel.initialize(function (error) {
            if (error && callback) callback(error);
        });
    });
};

ConsolidatorUiHost.prototype.liveReady = function () {
    return;
};

ConsolidatorUiHost.prototype.undo = function (callback) {
    return this.client.transactions.undo(callback);
};

ConsolidatorUiHost.prototype.redo = function (callback) {
    return this.client.transactions.redo(callback);
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

inlets = 2;
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

function handleListMessage(args) {
    if (args.length === 0) {
        return;
    }
    var selector = String(args.shift());
    if (inlet === 0 && selector === "track_name") {
        ensureHost().setTrackName(args);
        return;
    }
    if (inlet === 1) {
        if (args.length === 0) {
            return;
        }
        ensureHost().handleUiIntent(selector, String(args.shift()), args);
        return;
    }
    ensureHost().handleControl(selector, args);
}

function anything() {
    var args = arrayfromargs(arguments);
    if (inlet === 0) {
        if (messagename === "track_name") {
            ensureHost().setTrackName(args);
        } else {
            ensureHost().handleControl(messagename, args);
        }
    } else {
        var intent = args.shift();
        ensureHost().handleUiIntent(messagename, intent, args);
    }
}

function list() {
    handleListMessage(arrayfromargs(arguments));
}

function live_ready() {
    ensureHost().liveReady();
}

function undo() {
    ensureHost().undo();
}

function redo() {
    ensureHost().redo();
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
