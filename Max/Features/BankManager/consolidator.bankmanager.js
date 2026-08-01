autowatch = 1;
inlets = 2;
outlets = 3;

// Inlet 0: Host EQ, processor, and device snapshots.
// Inlet 1: global bank/link messages: bank.query, bank.announce, bank.leave,
// bank.reset_all, link.assign, link.detach, link.operation, link.filter_delta,
// link.filter_bypass, link.filter_reset, link.processor_detector_reset, link.state,
// link.processor_delta, link.processor_match, and link.processor_bypass.
// Outlet 0: Host commands: eq.select_bank, eq.join_banks, eq.commit_all,
// eq.reset_all, eq.set_link, gain.set_parameter, compressor.set_parameter,
// and saturator.set_parameter.
// Outlet 1: the complete global bank/link message set accepted by inlet 1.
// Outlet 2: link_color, processor_limits, processor_preview, detector_link_preview,
// eq_preview, and filter_limits UI state.
// Local gesture input also accepts eq_parameter_absolute_gesture
// <bankId> <filterId> <parameter> <absoluteValue> from SpectrumView and
// processor_match_operation <device> <onset|level> and
// processor_bypass_operation <compressor|saturator> <0|1> from processor controls.
// bank.announce: <instanceId> <trackName> <trackOrder> <revision> <selectedBank>
// <systemOccupied> <six bankId occupied linkId records>.
// link.assign|link.detach: <linkId> <instanceId> <bankId>.
// link.operation: <linkId> <sourceId> <revision> <join|commit|reset|bypass> <bypass|-1>.
// link.filter_delta: <linkId> <sourceId> <revision> <filterId> <parameterIndex> <normalizedDelta>.
// link.filter_bypass: <linkId> <sourceId> <revision> <filterId> <0|1>.
// link.filter_reset: <linkId> <sourceId> <revision> <filterId>.
// link.processor_delta: <linkId> <sourceId> <revision> <device> <parameter> <normalizedDelta>.
// link.processor_detector_reset: <linkId> <sourceId> <revision> <device> <filterId>.
// link.processor_match: <linkId> <sourceId> <revision> <device> <onset|level>.
// link.processor_bypass: <linkId> <sourceId> <revision> <compressor|saturator> <0|1>.
// link.state: <linkId> <sourceId> <revision> <bankId> <filterCount>
// <filterId> <bypass> <valueCount> <values...>... <processorValueCount>
// <device> <parameter> <absoluteValue>...

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
include("JS/BankManagerMath.js");
include("JS/BankManagerModels.js");
include("JS/BankManagerLiveIdentity.js");
include("JS/BankManagerSnapshotReader.js");
include("JS/BankManagerSnapshotCoordinator.js");
include("JS/BankManagerUiController.js");
include("JS/BankManagerMessageRouter.js");
include("JS/BankManagerDefinitions.js");
include("JS/BankManagerLinkGraph.js");
include("JS/BankManagerSelection.js");
include("JS/BankManagerLayout.js");
include("JS/BankManagerOperations.js");
include("JS/BankManagerLinkTransport.js");
include("JS/BankManagerLinkPresentation.js");
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
    this.linkEditingEnabled = false;
    this.clearAllConfirmationArmed = false;
    this.clearAllConfirmationTask = new Task(this.ClearAllConfirmationExpired, this);
    this.linkRevision = 0;
    this.linkRevisions = new LinkRevisionTracker();
    this.definitions = new BankManagerDefinitions();
    this.filterDefinitions = this.definitions.filterParameters;
    this.filterTypes = this.definitions.filterTypes;
    this.filterDefaultBypass = this.definitions.filterDefaultBypass;
    this.processorRanges = this.definitions.processorRanges;
    this.processorDefaults = this.definitions.processorDefaults;
    this.snapshotReader = new BankManagerSnapshotReader(this.filterDefinitions);
    this.snapshotCoordinator = new BankManagerSnapshotCoordinator(this);
    this.linkGraph = new BankManagerLinkGraph(this);
    this.layout = new BankManagerLayout();
    this.operations = new BankManagerOperations(this);
    this.linkTransport = new BankManagerLinkTransport(this);
    this.linkPresentation = new BankManagerLinkPresentation(this);
    this.controlLinkSession = "";
    this.pendingLinkedStatePublish = false;
    this.hasCanonicalEqSnapshot = false;
    this.viewModel = new BankManagerViewModel();
    this.renderer = new BankManagerRenderer();
    this.ui = new BankManagerUiController(this);
    this.messageRouter = new BankManagerMessageRouter(this);
    this.lastAnnouncementState = "";
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
    this.initializer.Dispose();
};

BankManager.prototype.TryInitialize = function() {
    var identity = this.liveIdentity.Resolve();
    if (!identity) return false;
    this.instanceId = identity.id;
    this.local.id = this.instanceId;
    this.local.label = identity.trackName;
    this.local.trackOrder = identity.trackOrder;
    this.liveIdentity.ObserveTrackName(identity.trackId);
    this.SetFocusedBank(this.local, this.local.selectedBankId);
    outlet(1, "bank.query", this.instanceId);
    this.PublishAnnouncement();
    this.PublishLinkedState();
    return true;
};

BankManager.prototype.HandleTrackNameChanged = function(values) {
    if (values.length !== 2 || String(values[0]) !== "name") return;
    var trackName = String(values[1] || "");
    if (!trackName || trackName === this.local.label) return;
    this.local.label = trackName;
    this.PublishAnnouncement();
    mgraphics.redraw();
};

BankManager.prototype.SendHostCommand = function(name, fields) {
    this.SendCommand(name, fields);
};

BankManager.prototype.ParseEqSnapshot = function(values) {
    return this.snapshotCoordinator.ApplyEq(values);
};

BankManager.prototype.ParseProcessorSnapshot = function(values) {
    return this.snapshotCoordinator.ApplyProcessor(values);
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
    if (instance.id !== this.instanceId) return;
    if (!this.selection.SetFocusedBank(instance, bankId)) return;
    if (!this.CanChangeFocusedBankLink()) this.linkEditingEnabled = false;
    this.controlLinkSession = "";
    // Local controls accept limits only for the selected bank confirmed by Host.
    if (instance.id === this.instanceId &&
        bankId === this.local.selectedBankId) {
        this.RefreshControlLinkSession();
    }
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

BankManager.prototype.SendDetectorReset = function(device, filterId) {
    var defaults = this.processorDefaults[device] || {};
    var parameters = ["gain", "frequency", "q", "bypass"];
    for (var index = 0; index < parameters.length; ++index) {
        var parameter = parameters[index];
        var key = "detector." + filterId + "." + parameter;
        if (!defaults.hasOwnProperty(key)) continue;
        this.SendHostCommand(device + ".set_detector_parameter", [
            Number(filterId), parameter, defaults[key]
        ]);
    }
};

BankManager.prototype.ResetDetectorModels = function(linkId, device, filterId) {
    var defaults = this.processorDefaults[device] || {};
    var group = linkId ? this.ProcessorLinkGroup(linkId, device) : null;
    var members = group ? group.members : {};
    if (!group && this.local.processors[device]) {
        members[this.instanceId] = this.local.processors[device];
    }
    var parameters = ["gain", "frequency", "q", "bypass"];
    for (var instanceId in members) {
        if (!members.hasOwnProperty(instanceId)) continue;
        var values = members[instanceId].values;
        for (var index = 0; index < parameters.length; ++index) {
            var key = "detector." + filterId + "." + parameters[index];
            if (defaults.hasOwnProperty(key)) values[key] = defaults[key];
        }
    }
};

BankManager.prototype.NormalizeLinkId = function(value) {
    var linkId = String(value || "");
    return linkId === "-" ? "" : linkId;
};

BankManager.prototype.RebuildProcessorLinkGroups = function() {
    this.linkGraph.Rebuild();
};

BankManager.prototype.ProcessorLinkGroup = function(linkId, device) {
    return this.linkGraph.ProcessorGroup(linkId, device);
};

BankManager.prototype.LinkMembers = function(linkId) {
    return this.linkGraph.Members(linkId);
};

BankManager.prototype.LinkMemberIds = function(linkId) {
    return this.linkGraph.MemberIds(linkId);
};

BankManager.prototype.RefreshControlLinkSession = function() {
    this.linkPresentation.RefreshControlSession();
};

BankManager.prototype.ParseDeviceSnapshot = function(values) {
    if (values.length !== 6 || String(values[0]) !== "snapshot" || String(values[3]) !== "device") return;
    if (!this.instanceId) return;
    this.PublishAnnouncement();
};

BankManager.prototype.ExecuteFilterReset = function(bankId, filterId) {
    this.operations.ResetFilter(bankId, filterId);
};

BankManager.prototype.ResetLinkedFilterModels = function(linkId, filterId) {
    this.operations.ResetLinkedFilterModels(linkId, filterId);
};

BankManager.prototype.ResetFilterModel = function(bank, filterId) {
    this.operations.ResetFilterModel(bank, filterId);
};

BankManager.prototype.FindLocalLinkedBank = function(linkId) {
    return this.linkGraph.FindLocalBank(linkId);
};

BankManager.prototype.LocalBank = function(bankId) {
    return this.local.banks[bankId - 1] || null;
};

BankManager.prototype.Rows = function() {
    return this.selection.Rows();
};

BankManager.prototype.DisplayRows = function() {
    return this.Rows();
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
    this.operations.ResetAllModels();
    this.linkPresentation.ClearPreviews();
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

BankManager.prototype.IsEditBankSelected = function(instance, bank) {
    return this.selection.IsEditSelected(instance, bank);
};

BankManager.prototype.IsEditableBank = function(bank) {
    return Boolean(bank && bank.id >= 2 && bank.id <= 5);
};

BankManager.prototype.CanAssignBankToLink = function(instance, bank, linkId, reservedInstances) {
    if (!instance || !this.IsEditableBank(bank) || bank.linkId ||
        this.EditableLinkIds().indexOf(linkId) < 0) return false;
    if (reservedInstances && reservedInstances[instance.id]) return false;
    for (var index = 0; index < instance.banks.length; ++index) {
        var candidate = instance.banks[index];
        if (candidate.id !== bank.id && candidate.linkId === linkId) return false;
    }
    return true;
};

BankManager.prototype.EditBankMembership = function(instance, bank, extend) {
    if (!this.linkEditingEnabled || !this.IsEditableBank(bank)) return;
    this.selection.ToggleEditBank(instance, bank, extend);
};

BankManager.prototype.EditSelection = function() {
    return this.selection.EditSelection();
};

BankManager.prototype.CanAssignEditSelectionToLink = function(linkId) {
    var members = this.EditSelection();
    if (members.length === 0 || this.EditableLinkIds().indexOf(linkId) < 0) return false;
    var reservedInstances = {};
    for (var index = 0; index < members.length; ++index) {
        var member = members[index];
        if (!this.CanAssignBankToLink(member.instance, member.bank, linkId, reservedInstances)) {
            return false;
        }
        reservedInstances[member.instance.id] = true;
    }
    return true;
};

BankManager.prototype.EditSelectionForLink = function(linkId) {
    return this.EditSelection().filter(function(member) {
        return member.bank.linkId === linkId;
    });
};

BankManager.prototype.CanDetachEditSelectionFromLink = function(linkId) {
    var members = this.EditSelection();
    if (members.length === 0) return false;
    for (var index = 0; index < members.length; ++index) {
        if (members[index].bank.linkId !== linkId) return false;
    }
    return true;
};

BankManager.prototype.CanApplyEditSelectionToLink = function(linkId) {
    return this.CanDetachEditSelectionFromLink(linkId) ||
        this.CanAssignEditSelectionToLink(linkId);
};

BankManager.prototype.HasEditSelectionInLink = function(linkId) {
    return this.CanDetachEditSelectionFromLink(linkId);
};

BankManager.prototype.AssignEditSelection = function(linkId) {
    if (!this.linkEditingEnabled || !this.CanAssignEditSelectionToLink(linkId)) return;
    var members = this.EditSelection();
    for (var index = 0; index < members.length; ++index) {
        var member = members[index];
        if (member.instance.id === this.instanceId) {
            this.SendHostCommand("eq.set_link", [member.bank.id, linkId]);
        } else {
            outlet(1, "link.assign", linkId, member.instance.id, member.bank.id);
        }
    }
};

BankManager.prototype.DetachEditSelectionFromLink = function(linkId) {
    if (!this.CanDetachEditSelectionFromLink(linkId)) return false;
    var members = this.EditSelectionForLink(linkId);
    for (var index = 0; index < members.length; ++index) {
        var member = members[index];
        if (member.instance.id === this.instanceId) {
            this.SendHostCommand("eq.set_link", [member.bank.id, "-"]);
        } else {
            outlet(1, "link.detach", linkId, member.instance.id, member.bank.id);
        }
    }
    return true;
};

BankManager.prototype.ApplyEditSelectionToLink = function(linkId) {
    if (!this.linkEditingEnabled) return;
    if (this.DetachEditSelectionFromLink(linkId)) return;
    this.AssignEditSelection(linkId);
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

BankManager.prototype.ApplyLinkOperation = function(values) {
    this.operations.ApplyLinked(values);
};

BankManager.prototype.ResetLinkedBankModels = function(linkId) {
    var members = this.LinkMembers(linkId);
    for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
        var filters = members[memberIndex].bank.filters;
        for (var filterId in filters) {
            if (filters.hasOwnProperty(filterId)) {
                this.ResetFilterModel(members[memberIndex].bank, Number(filterId));
            }
        }
    }
};

// The root owns feature composition; LinkTransport owns every global frame.
BankManager.prototype.NextLinkRevision = function(linkId) {
    return this.linkTransport.NextRevision(linkId);
};

BankManager.prototype.PublishEqPreview = function(bankId, filterId, parameterIndex, absoluteValue) {
    this.linkTransport.PublishEqPreview(bankId, filterId, parameterIndex, absoluteValue);
};

BankManager.prototype.PublishProcessorPreview = function(device, parameter, absoluteValue) {
    this.linkTransport.PublishProcessorPreview(device, parameter, absoluteValue);
};

BankManager.prototype.ApplyFilterDeltaToModel = function(update, skipInstanceId) {
    this.linkTransport.ApplyFilterDeltaToModel(update, skipInstanceId);
};

BankManager.prototype.PublishLinkBypass = function(linkId, filterId, bypass) {
    this.linkTransport.PublishFilterBypass(linkId, filterId, bypass);
};

BankManager.prototype.PublishAnnouncement = function() {
    return this.linkTransport.PublishAnnouncement();
};

BankManager.prototype.PublishLinkedState = function(linkIds) {
    this.linkTransport.PublishLinkedState(linkIds);
};

BankManager.prototype.ParseAnnouncement = function(values) {
    this.linkTransport.ParseAnnouncement(values);
};

BankManager.prototype.ApplyLinkState = function(values) {
    this.linkTransport.ApplyLinkedState(values);
};

BankManager.prototype.HandleGlobal = function(name, values) {
    this.linkTransport.HandleGlobal(name, values);
};

BankManager.prototype.ApplyProcessorDelta = function(values) {
    this.linkTransport.ApplyProcessorDelta(values);
};

BankManager.prototype.ApplyProcessorDetectorReset = function(values) {
    this.linkTransport.ApplyDetectorReset(values);
};

BankManager.prototype.RemovePeer = function(values) {
    this.linkTransport.RemovePeer(values);
};

BankManager.prototype.ApplyFilterDelta = function(values) {
    this.linkTransport.ApplyFilterDelta(values);
};

BankManager.prototype.ApplyLinkBypass = function(values) {
    this.linkTransport.ApplyFilterBypass(values);
};

BankManager.prototype.ApplyLinkFilterReset = function(values) {
    this.linkTransport.ApplyFilterReset(values);
};

BankManager.prototype.HandleEqAbsoluteParameterGesture = function(values) {
    this.linkTransport.HandleEqGesture(values);
};

BankManager.prototype.HandleEqAbsoluteParameterPreview = function(values) {
    this.linkTransport.HandleEqPreview(values);
};

BankManager.prototype.HandleProcessorParameterGesture = function(values) {
    this.linkTransport.HandleProcessorGesture(values);
};

BankManager.prototype.HandleProcessorMatchOperation = function(values) {
    if (values.length !== 2) return;
    var device = String(values[0]);
    var operation = String(values[1]);
    if (["input_gain", "output_gain", "compressor", "saturator"].indexOf(device) < 0 ||
        (operation !== "onset" && operation !== "level")) return;
    if ((device === "input_gain" || device === "output_gain") && operation !== "level") return;
    this.linkTransport.StartProcessorMatch(device, operation);
};

BankManager.prototype.HandleProcessorBypassOperation = function(values) {
    if (values.length !== 2) return;
    var device = String(values[0]);
    var bypass = Number(values[1]);
    if ((device !== "compressor" && device !== "saturator") ||
        (bypass !== 0 && bypass !== 1)) return;
    this.linkTransport.StartProcessorBypass(device, bypass);
};

BankManager.prototype.HandleProcessorDetectorReset = function(values) {
    this.linkTransport.HandleDetectorReset(values);
};

BankManager.prototype.PublishFilterLimits = function(linkId, isLinked) {
    this.linkPresentation.PublishFilterLimits(linkId, isLinked);
};

BankManager.prototype.PublishLinkedDetectorPreviews = function(linkId, isLinked) {
    this.linkPresentation.PublishDetectorPreviews(linkId, isLinked);
};

BankManager.prototype.PublishLinkedFilterPreviews = function(linkId, isLinked) {
    this.linkPresentation.PublishFilterPreviews(linkId, isLinked);
};

BankManager.prototype.PublishChangedFilterPreviews = function(linkId, filterId) {
    this.linkPresentation.PublishChangedFilterPreviews(linkId, filterId);
};

var bankManager = new BankManager();

function BankManagerTrackNameChanged() {
    var values = arguments.length === 1 && arguments[0] instanceof Array
        ? arguments[0]
        : arrayfromargs(arguments);
    bankManager.HandleTrackNameChanged(values);
}

function inletassist(index) {
    assist(index === 0
        ? "Local input: Host snapshots; eq_parameter_absolute_gesture, eq_parameter_absolute_preview, eq_filter_reset, processor_parameter_gesture, processor_match_operation, processor_bypass_operation"
        : "Global bus: bank.query, bank.announce, bank.leave, bank.reset_all, link.assign, link.detach, link.operation, link.filter_*, link.processor_*, link.state");
}

function outletassist(index) {
    assist([
        "Host commands: eq.select_bank, eq.join_banks, eq.commit_all, eq.reset_all, eq.set_link, gain.set_parameter, compressor.set_parameter, saturator.set_parameter",
        "Global bus: bank.query, bank.announce, bank.leave, bank.reset_all, link.assign, link.detach, link.operation, link.filter_*, link.processor_*, link.state",
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
function snapshot() {
    if (inlet === 0) {
        bankManager.messageRouter.HandleSnapshot(
            ["snapshot"].concat(arrayfromargs(arguments)));
        mgraphics.redraw();
    }
}
function event() {}
function eq_parameter_absolute_gesture() {
    if (inlet === 0) {
        bankManager.messageRouter.HandleLocal(
            "eq_parameter_absolute_gesture", arrayfromargs(arguments));
    }
}
function eq_parameter_absolute_preview() {
    if (inlet === 0) {
        bankManager.messageRouter.HandleLocal(
            "eq_parameter_absolute_preview", arrayfromargs(arguments));
    }
}
function processor_parameter_gesture() {
    if (inlet === 0) {
        bankManager.messageRouter.HandleLocal(
            "processor_parameter_gesture", arrayfromargs(arguments));
    }
}
function processor_detector_reset() {
    if (inlet === 0) {
        bankManager.messageRouter.HandleLocal(
            "processor_detector_reset", arrayfromargs(arguments));
    }
}
function eq_filter_reset(bankId, filterId) {
    if (inlet === 0) bankManager.messageRouter.HandleLocal(
        "eq_filter_reset", [bankId, filterId]);
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
    var values = arrayfromargs(arguments);
    if (inlet === 0 && values.length && String(values[0]) === "snapshot") {
        bankManager.messageRouter.HandleSnapshot(values);
    }
}
function leave() {
    if (bankManager.instanceId) outlet(1, "bank.leave", bankManager.instanceId);
}
function notifydeleted() { bankManager.Dispose(); }
