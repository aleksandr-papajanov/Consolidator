autowatch = 1;
inlets = 2;
outlets = 3;

// Inlet 0: runtime history events, coordinator directory, and local operations.
// Inlet 1: global coordinator and discrete group-operation messages.
// Outlet 0: Host commands: eq.select_bank, eq.join_banks, eq.commit_all,
// eq.reset_all, eq.set_link, gain.set_parameter, compressor.set_parameter,
// and saturator.set_parameter.
// Outlet 1: global coordinator and discrete group-operation messages.
// Outlet 2: link_color, processor_limits, processor_preview, eq_preview, and
// filter_limits UI state.
// Local operation input accepts processor_match_operation <device> <onset|level> and
// processor_bypass_operation <compressor|saturator> <0|1> from processor controls.
// link.assign|link.detach: <linkId> <instanceId> <bankId>.
// link.operation: <linkId> <sourceId> <revision> <join|commit|reset|bypass> <bypass|-1>.
// link.filter_bypass: <linkId> <sourceId> <revision> <FilterId> <0|1>.
// link.filter_reset: <linkId> <sourceId> <revision> <FilterId>.
// link.processor_detector_reset: <linkId> <sourceId> <revision> <device> <FilterId>.
// link.processor_match: <linkId> <sourceId> <revision> <device> <onset|level>.
// link.processor_bypass: <linkId> <sourceId> <revision> <compressor|saturator> <0|1>.

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;
include("../../Shared/Runtime/LiveApiInitializer.js");
include("../../Shared/Runtime/ControlControllerBase.js");
include("../../Shared/Runtime/LinkRevisionTracker.js");
include("../../Shared/Interface/BankManager/BankManagerOptions.js");
include("../../Shared/Interface/BankManager/BankManagerViewModel.js");
include("../../Shared/Interface/BankManager/BankManagerRenderer.js");
include("../../Shared/Interface/ButtonGroup/ButtonGroupLayout.js");
include("../../Shared/Configuration/FilterDefinitions.js");
include("JS/BankManagerModels.js");
include("JS/BankManagerLiveIdentity.js");
include("JS/BankManagerUiController.js");
include("JS/BankManagerMessageRouter.js");
include("JS/BankManagerSelection.js");
include("JS/BankVisibilityPolicy.js");
include("JS/GroupOperationPlanner.js");
include("JS/LinkMutationDispatcher.js");
include("JS/BankManagerLayout.js");
include("JS/BankManagerOperations.js");
var BankManagerVisualOptions = BankManagerOptions.geometry;
var BankManagerColors = BankManagerOptions.colors;
var bankGroupLayout = new ButtonGroupLayout();

function BankManager() {
    box.message("border", 0);
    ControlControllerBase.call(this, "bankmanager.ui", null, this);
    this.instanceId = "";
    this.local = new InstanceSummary("", "Consolidator");
    this.peers = {};
    this.selection = new BankManagerSelection(this);
    this.visibilityPolicy = new BankVisibilityPolicy(this);
    this.groupOperations = new GroupOperationPlanner(this);
    this.linkMutations = new LinkMutationDispatcher(this);
    this.linkEditingEnabled = false;
    this.clearAllConfirmationArmed = false;
    this.clearAllConfirmationTask = new Task(this.ClearAllConfirmationExpired, this);
    this.linkRevision = 0;
    this.linkRevisions = new LinkRevisionTracker();
    this.layout = new BankManagerLayout();
    this.operations = new BankManagerOperations(this);
    this.controlLinkSession = "";
    this.viewModel = new BankManagerViewModel();
    this.renderer = new BankManagerRenderer();
    this.ui = new BankManagerUiController(this);
    this.messageRouter = new BankManagerMessageRouter(this);
    this.liveIdentity = new BankManagerLiveIdentity();
    this.initializer = new LiveApiInitializer(
        this.TryInitialize, this, 50);
}

BankManager.prototype = Object.create(ControlControllerBase.prototype);
BankManager.prototype.constructor = BankManager;

BankManager.prototype.Initialize = function() {
    this.initializer.Start();
};

BankManager.prototype.Dispose = function() {
    this.clearAllConfirmationTask.cancel();
    this.linkMutations.Dispose();
    this.initializer.Dispose();
};

BankManager.prototype.TryInitialize = function() {
    var identity = this.liveIdentity.Resolve();
    if (!identity) return false;
    this.instanceId = identity.id;
    this.local.id = this.instanceId;
    this.local.label = identity.trackName;
    this.local.trackOrder = identity.trackOrder;
    this.SetFocusedBank(this.local, this.local.selectedBankId);
    return true;
};

BankManager.prototype.SendHostCommand = function(name, fields) {
    this.SendCommand(name, fields);
};

BankManager.prototype.QueueLinkMutation = function(instance, bank, linkId) {
    this.linkMutations.Enqueue(instance, bank, linkId);
};

BankManager.prototype.HandleRuntimeEvent = function(values) {
    this.operations.HandleHistoryEvent(values);
};

BankManager.prototype.ActiveBank = function(instance) {
    return this.selection.ActiveBank(instance);
};

BankManager.prototype.ActiveLinkId = function(instance) {
    return this.selection.ActiveLinkId(instance);
};

BankManager.prototype.FocusedInstance = function() {
    return this.selection.FocusedInstance();
};

BankManager.prototype.FocusedBank = function() {
    return this.selection.FocusedBank();
};

BankManager.prototype.IsFocusedBank = function(instance, bank) {
    return Boolean(instance && bank &&
        instance.id === this.FocusedInstance().id &&
        bank.id === this.selection.focusedBankId);
};

BankManager.prototype.SetFocusedBank = function(instance, bankId) {
    if (!instance || !isFinite(bankId) || bankId < 1 || bankId > 6) return;
    if (!this.selection.SetFocusedBank(instance, bankId)) return;
    if (!this.CanChangeFocusedBankLink()) this.linkEditingEnabled = false;
    this.controlLinkSession = "";
    // Local controls accept limits only for the selected bank confirmed by Host.
    this.SynchronizeCoordinator();
};

BankManager.prototype.HasLink = function(instance, linkId) {
    if (!instance || !linkId) return false;
    for (var index = 0; index < instance.banks.length; index++) {
        if (instance.banks[index].linkId === linkId) return true;
    }
    return false;
};

BankManager.prototype.SendProcessorValue = function(device, parameter, value) {
    if (device === "input_gain" || device === "output_gain") {
        this.SendHostCommand("gain.set_parameter", [device === "input_gain" ? "input" : "output", value]);
    } else if ((device === "compressor" || device === "saturator") &&
        parameter.indexOf("detector.") === 0) {
        var parts = parameter.split(".");
        if (parts.length !== 3) return;
        this.SendHostCommand(device + ".set_detector_parameter", [
            Number(parts[1]), String(parts[2]), value
        ]);
    } else {
        this.SendHostCommand(device + ".set_parameter", [parameter, value]);
    }
};

BankManager.prototype.SendProcessorBypass = function(device, bypass) {
    if (device !== "compressor" && device !== "saturator") return;
    this.SendHostCommand(device + ".set_bypass", [Number(bypass) !== 0 ? 1 : 0]);
};

BankManager.prototype.SendDetectorReset = function(device, FilterId) {
    var definition = FilterDefinitionCatalog.Processors()[device];
    if (!definition) return;
    var parameters = ["gain", "frequency", "q", "bypass"];
    for (var index = 0; index < parameters.length; ++index) {
        var parameter = parameters[index];
        var key = "detector." + FilterId + "." + parameter;
        var defaultValue;
        for (var parameterIndex = 0;
             parameterIndex < definition.parameters.length;
             ++parameterIndex) {
            var candidate = definition.parameters[parameterIndex];
            if (candidate.name === key) {
                defaultValue = candidate.defaultValue;
                break;
            }
        }
        if (!isFinite(defaultValue)) continue;
        this.SendHostCommand(device + ".set_detector_parameter", [
            Number(FilterId), parameter, defaultValue
        ]);
    }
};

BankManager.prototype.NormalizeLinkId = function(value) {
    var linkId = String(value || "");
    return linkId === "-" ? "" : linkId;
};

BankManager.prototype.LinkMembers = function(linkId) {
    var members = [];
    var instances = [this.local];
    for (var runtimeId in this.peers) {
        if (this.peers.hasOwnProperty(runtimeId)) instances.push(this.peers[runtimeId]);
    }
    for (var instanceIndex = 0; instanceIndex < instances.length; ++instanceIndex) {
        var banks = instances[instanceIndex].banks;
        for (var bankIndex = 0; bankIndex < banks.length; ++bankIndex) {
            if (banks[bankIndex].linkId === String(linkId)) {
                members.push({ instance: instances[instanceIndex], bank: banks[bankIndex] });
            }
        }
    }
    return members;
};

BankManager.prototype.LinkMemberIds = function(linkId) {
    return this.LinkMembers(linkId).map(function(member) {
        return member.instance.id;
    }).sort();
};

BankManager.prototype.RefreshControlLinkSession = function() {
    var focusedInstance = this.FocusedInstance();
    var focusedBank = this.FocusedBank();
    var activeLinkId = focusedBank ? focusedBank.linkId : "";
    var activeMembers = activeLinkId ? this.LinkMemberIds(activeLinkId) : [];
    var signature = (activeLinkId && activeMembers.length >= 2
        ? activeLinkId + ":" + activeMembers.join(",")
        : "unlinked") + ":" + (focusedBank ? focusedBank.id : 0) + ":" + focusedInstance.id;
    if (this.controlLinkSession === signature) return;
    this.controlLinkSession = signature;
    var color = activeLinkId ? this.LinkColor(activeLinkId) : null;
    outlet(2, "link_color", activeLinkId && color ? activeLinkId : "-",
        color ? color[0] : 0, color ? color[1] : 0,
        color ? color[2] : 0, color ? color[3] : 0);
    outlet(0, "coordinator_select_target", focusedInstance.id, focusedBank.id);
    outlet(0, "coordinator_limits");
};

BankManager.prototype.ParseCoordinatorDirectory = function(values) {
    var count = Number(values[0]);
    if (!isFinite(count) || count < 0) return;
    var position = 1;
    var peers = {};
    var previousSelectedBankId = this.local.selectedBankId;
    for (var index = 0; index < count; ++index) {
        if (position + 18 > values.length) return;
        var runtimeId = String(values[position++]);
        var label = String(values[position++]);
        var trackOrder = Number(values[position++]);
        var selectedBankId = Number(values[position++]);
        if (!runtimeId || !isFinite(trackOrder) ||
            selectedBankId < 1 || selectedBankId > 6) return;
        var instance = runtimeId === this.instanceId ? this.local :
            new InstanceSummary(runtimeId, label);
        instance.label = label;
        instance.trackOrder = trackOrder;
        instance.selectedBankId = selectedBankId;
        for (var bankId = 0; bankId <= 6; ++bankId) {
            var linkId = String(values[position++]);
            var occupied = Number(values[position++]) !== 0;
            var bank = bankId === 0 ? instance.systemBank : instance.banks[bankId - 1];
            bank.linkId = linkId === "-" ? "" : linkId;
            bank.occupied = occupied;
        }
        if (runtimeId !== this.instanceId) peers[runtimeId] = instance;
    }
    this.peers = peers;
    if (this.local.selectedBankId !== previousSelectedBankId) {
        this.selection.SetFocusedBank(this.local, this.local.selectedBankId);
    }
    this.controlLinkSession = "";
    this.RefreshControlLinkSession();
    mgraphics.redraw();
};

BankManager.prototype.ExecuteFilterReset = function(bankId, FilterId) {
    this.operations.ResetFilter(bankId, FilterId);
};

BankManager.prototype.FindLocalLinkedBank = function(linkId) {
    var active = this.local.banks[this.local.selectedBankId - 1];
    if (active && active.linkId === linkId) return active;
    for (var index = 0; index < this.local.banks.length; ++index) {
        if (this.local.banks[index].linkId === linkId) return this.local.banks[index];
    }
    return null;
};

BankManager.prototype.LocalBank = function(bankId) {
    return this.local.banks[bankId - 1] || null;
};

BankManager.prototype.Rows = function() {
    return this.selection.Rows();
};

BankManager.prototype.DisplayRows = function() {
    return this.visibilityPolicy.Rows();
};

BankManager.prototype.BankStartX = function(width) {
    return this.layout.BankStartX(width);
};

BankManager.prototype.BankColumnWidth = function(width) {
    return this.layout.BankColumnWidth(width);
};

BankManager.prototype.ContentHeight = function() {
    return this.layout.ContentHeight(mgraphics.size[1]);
};

BankManager.prototype.MaximumScrollOffset = function() {
    this.viewModel.listView.SetItems(this.DisplayRows());
    return this.viewModel.listView.MaximumScrollOffset(
        this.ContentHeight(), BankManagerVisualOptions.rowHeight);
};

BankManager.prototype.EditableLinkIds = function() {
    return this.layout.EditableLinkIds();
};

BankManager.prototype.LinkColor = function(linkId) {
    return this.layout.LinkColor(linkId);
};

BankManager.prototype.LinkPanelRect = function(width, height) {
    return this.layout.LinkPanelRect(width, height);
};

BankManager.prototype.ClearAllRect = function(width) {
    return this.layout.ClearAllRect(width);
};

BankManager.prototype.LinkEditRect = function(width) {
    return this.layout.LinkEditRect(width);
};

BankManager.prototype.IsPointInRect = function(x, y, rect) {
    return this.layout.Contains(x, y, rect);
};

BankManager.prototype.ToggleLinkEditing = function() {
    if (!this.CanChangeFocusedBankLink()) return;
    this.linkEditingEnabled = !this.linkEditingEnabled;
    this.selection.ClearEditSelection();
    mgraphics.redraw();
};

BankManager.prototype.ClearAllEqBanks = function() {
    if (!this.instanceId) return;
    if (!this.clearAllConfirmationArmed) {
        this.clearAllConfirmationArmed = true;
        this.clearAllConfirmationTask.cancel();
        this.clearAllConfirmationTask.schedule(
            BankManagerVisualOptions.clearAllConfirmTimeoutMs);
        mgraphics.redraw();
        return;
    }
    this.clearAllConfirmationTask.cancel();
    this.clearAllConfirmationArmed = false;
    this.ResetAllBankModels();
    outlet(1, "bank.reset_all", this.instanceId);
    mgraphics.redraw();
};

BankManager.prototype.ClearAllConfirmationExpired = function() {
    this.clearAllConfirmationArmed = false;
    mgraphics.redraw();
};

BankManager.prototype.ResetAllBankModels = function() {
};

BankManager.prototype.LinkGroupIndexAt = function(x, y, width, height) {
    return this.layout.LinkGroupIndexAt(x, y, width, height);
};

BankManager.prototype.FocusedLinkId = function() {
    var bank = this.FocusedBank();
    return bank ? bank.linkId : "";
};

BankManager.prototype.ActiveEditableLinkId = function() {
    var linkId = this.FocusedLinkId();
    return this.EditableLinkIds().indexOf(linkId) >= 0 ? linkId : "";
};

BankManager.prototype.CanChangeFocusedBankLink = function() {
    var bank = this.FocusedBank();
    return Boolean(bank && bank.id >= 2 && bank.id <= 5);
};

BankManager.prototype.IsActiveGroupMember = function(bank) {
    var linkId = this.ActiveLinkId(this.local);
    return Boolean(linkId && bank && bank.linkId === linkId);
};

BankManager.prototype.LinkMemberCount = function(linkId) {
    return this.LinkMembers(linkId).length;
};

BankManager.prototype.ApplyLinkAssignment = function(values) {
    this.operations.Assign(values);
};

BankManager.prototype.ApplyLinkDetachment = function(values) {
    this.operations.Detach(values);
};

BankManager.prototype.ExecuteOperation = function(action, bypass) {
    this.operations.Execute(action, bypass);
};

BankManager.prototype.ApplyOperation = function(action, bankId, bypass) {
    this.operations.Apply(action, bankId, bypass);
};

BankManager.prototype.NextLinkRevision = function(linkId) {
    return this.operations.NextRevision(linkId);
};

BankManager.prototype.PublishEqPreview = function(bankId, FilterId, parameterIndex, absoluteValue) {
    outlet(2, "eq_preview", Number(bankId), Number(FilterId),
        Number(parameterIndex), Number(absoluteValue));
};

BankManager.prototype.PublishProcessorPreview = function(device, parameter, absoluteValue) {
    outlet(2, "processor_preview", String(device), String(parameter),
        Number(absoluteValue));
};

BankManager.prototype.PublishLinkBypass = function(linkId, FilterId, bypass) {
    this.operations.PublishFilterBypass(linkId, FilterId, bypass);
};

BankManager.prototype.SynchronizeCoordinator = function() {
    this.controlLinkSession = "";
    this.RefreshControlLinkSession();
    mgraphics.redraw();
};

BankManager.prototype.HandleGlobal = function(name, values) {
    this.operations.HandleGlobal(name, values);
};

BankManager.prototype.HandleProcessorMatchOperation = function(values) {
    if (values.length !== 2) return;
    var device = String(values[0]);
    var operation = String(values[1]);
    if (["input_gain", "output_gain", "compressor", "saturator"].indexOf(device) < 0 ||
        (operation !== "onset" && operation !== "level")) return;
    if ((device === "input_gain" || device === "output_gain") && operation !== "level") return;
    this.operations.StartProcessorMatch(device, operation);
};

BankManager.prototype.HandleProcessorBypassOperation = function(values) {
    if (values.length !== 2) return;
    var device = String(values[0]);
    var bypass = Number(values[1]);
    if ((device !== "compressor" && device !== "saturator") ||
        (bypass !== 0 && bypass !== 1)) return;
    this.operations.StartProcessorBypass(device, bypass);
};

BankManager.prototype.HandleProcessorDetectorReset = function(values) {
    this.operations.HandleDetectorReset(values);
};

var bankManager = new BankManager();

function inletassist(index) {
    assist(index === 0
        ? "Local input: coordinator directory, runtime history events, filter reset, and processor operations"
        : "Global bus: coordinator and discrete linked operations");
}

function outletassist(index) {
    assist([
        "Host commands: eq.select_bank, eq.join_banks, eq.commit_all, eq.reset_all, eq.set_link, gain.set_parameter, compressor.set_parameter, saturator.set_parameter",
        "Global bus: coordinator and discrete linked operations",
        "Local link UI: link_color, processor_limits, eq_preview, filter_limits, processor_preview"
    ][index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function loadbang() { bankManager.Initialize(); }
function initialize() { bankManager.Initialize(); }
function paint() { bankManager.ui.Paint(); }
function onclick(x, y, button, cmd, shift) {
    bankManager.ui.Click(x, y, Boolean(shift));
}
function onwheel(x, y, scrollx, scrolly) { bankManager.ui.Scroll(scrolly); }
function snapshot() {}
function event() {
    if (inlet === 0) bankManager.HandleRuntimeEvent(arrayfromargs(arguments));
}
function processor_detector_reset() {
    if (inlet === 0) {
        bankManager.messageRouter.HandleLocal(
            "processor_detector_reset", arrayfromargs(arguments));
    }
}
function eq_filter_reset(bankId, FilterId) {
    if (inlet === 0) bankManager.messageRouter.HandleLocal(
        "eq_filter_reset", [bankId, FilterId]);
}
function bank_action(action, value) {
    if (inlet !== 0) return;
    var name = String(action);
    if (name !== "join" && name !== "commit" && name !== "reset" && name !== "bypass") return;
    bankManager.messageRouter.HandleLocal("bank.action", [name, value]);
}
function anything() {
    var values = arrayfromargs(arguments);
    bankManager.messageRouter.Handle(inlet, messagename, values);
}
function list() {
}
function leave() {}
function notifydeleted() { bankManager.Dispose(); }
