autowatch = 1;
inlets = 2;
outlets = 3;

// Inlet 0: Host EQ, processor, and device snapshots.
// Inlet 1: global bank/link messages: bank.query, bank.announce, bank.leave,
// bank.reset_all, link.assign, link.detach, link.operation, link.filter_delta,
// link.filter_bypass, link.filter_reset, link.processor_detector_reset, link.state,
// and link.processor_delta.
// Outlet 0: Host commands: eq.select_bank, eq.join_banks, eq.commit_all,
// eq.reset_all, eq.set_link, gain.set_parameter, compressor.set_parameter,
// and saturator.set_parameter.
// Outlet 1: the complete global bank/link message set accepted by inlet 1.
// Outlet 2: link_color, processor_limits, processor_preview, detector_link_preview,
// eq_preview, and filter_limits UI state.
// Local gesture input also accepts eq_parameter_absolute_gesture
// <bankId> <filterId> <parameter> <absoluteValue> from SpectrumView.
// bank.announce: <instanceId> <trackName> <trackOrder> <revision> <selectedBank>
// <systemOccupied> <six bankId occupied linkId records>.
// link.assign|link.detach: <linkId> <instanceId> <bankId>.
// link.operation: <linkId> <sourceId> <revision> <join|commit|reset|bypass> <bypass|-1>.
// link.filter_delta: <linkId> <sourceId> <revision> <filterId> <parameterIndex> <normalizedDelta>.
// link.filter_bypass: <linkId> <sourceId> <revision> <filterId> <0|1>.
// link.filter_reset: <linkId> <sourceId> <revision> <filterId>.
// link.processor_delta: <linkId> <sourceId> <revision> <device> <parameter> <normalizedDelta>.
// link.processor_detector_reset: <linkId> <sourceId> <revision> <device> <filterId>.
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
include("JS/BankManagerUiController.js");
include("JS/BankManagerMessageRouter.js");
include("JS/BankManagerDefinitions.js");
include("JS/BankManagerLinkGraph.js");
include("JS/BankManagerSelection.js");
include("JS/BankManagerLayout.js");
include("JS/BankManagerOperations.js");
include("JS/BankManagerLinkTransport.js");
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
    this.linkGraph = new BankManagerLinkGraph(this);
    this.layout = new BankManagerLayout();
    this.operations = new BankManagerOperations(this);
    this.linkTransport = new BankManagerLinkTransport(this);
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
    var state = this.snapshotReader.ReadEq(values);
    if (!state) return false;
    var previousSelectedBankId = this.local.selectedBankId;
    var previousLinkIds = {};
    for (var previousIndex = 0; previousIndex < this.local.banks.length; previousIndex++) {
        var previousLinkId = this.local.banks[previousIndex].linkId;
        if (previousLinkId) previousLinkIds[previousIndex + 1] = previousLinkId;
    }
    this.local.revision = state.revision;
    this.local.eqBypass = state.bypass;
    this.local.selectedBankId = state.selectedBankId;
    this.local.systemBank = state.systemBank;
    this.local.banks = state.banks;
    var changedLinkIds = {};
    var linkTopologyChanged = false;
    for (var bankIndex = 0; bankIndex < state.banks.length; bankIndex++) {
        var previousLinkId = previousLinkIds[bankIndex + 1] || "";
        var currentLinkId = state.banks[bankIndex].linkId || "";
        if (previousLinkId === currentLinkId) continue;
        linkTopologyChanged = true;
        if (currentLinkId) changedLinkIds[currentLinkId] = true;
    }
    if (linkTopologyChanged) {
        this.RebuildProcessorLinkGroups();
        if (this.selection.focusedInstanceId === this.instanceId) {
            this.controlLinkSession = "";
            this.RefreshControlLinkSession();
        }
    }
    if ((!this.selection.focusedInstanceId ||
        this.selection.focusedInstanceId === this.instanceId) &&
        (!this.hasCanonicalEqSnapshot || previousSelectedBankId !== state.selectedBankId)) {
        this.SetFocusedBank(this.local, state.selectedBankId);
    }
    this.hasCanonicalEqSnapshot = true;
    this.PublishAnnouncement();
    this.PublishLinkedState(changedLinkIds);
    if (this.pendingLinkedStatePublish) {
        this.pendingLinkedStatePublish = false;
        this.PublishLinkedState();
    }
    var activeLinkId = this.ActiveLinkId(this.local);
    this.PublishLinkedFilterPreviews(
        activeLinkId,
        activeLinkId && this.LinkMemberIds(activeLinkId).length >= 2
    );
    return true;
};

BankManager.prototype.ParseProcessorSnapshot = function(values) {
    var processors = this.snapshotReader.ReadProcessor(values);
    if (!processors) return;
    var shouldAnnounce = !isFinite(
        this.local.processors.input_gain.values.gain);
    for (var device in processors) {
        if (!processors.hasOwnProperty(device)) continue;
        this.local.processors[device].values = processors[device].values;
    }
    if (shouldAnnounce) {
        this.controlLinkSession = "";
        this.PublishAnnouncement();
        this.PublishLinkedState();
    }
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

BankManager.prototype.PublishFilterLimits = function(linkId, isLinked) {
    var source = this.ActiveBank(this.local);
    if (!source) return;
    var members = isLinked ? this.LinkMembers(linkId) : [];
    for (var filterId in this.filterDefinitions) {
        if (!this.filterDefinitions.hasOwnProperty(filterId)) continue;
        var parameters = this.filterDefinitions[filterId];
        var sourceFilter = source.filters[filterId];
        if (!sourceFilter) continue;
        for (var parameterIndex = 0;
             parameterIndex < parameters.length;
             ++parameterIndex) {
            var definition = parameters[parameterIndex];
            var sourceValue = BankManagerMath.Normalize(
                sourceFilter.values[parameterIndex], definition);
            if (!isFinite(sourceValue)) continue;
            var minimumDelta = isLinked ? -Infinity : -sourceValue;
            var maximumDelta = isLinked ? Infinity : 1 - sourceValue;
            for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
                if (members[memberIndex].instance.id === this.instanceId) continue;
                var filter = members[memberIndex].bank.filters[filterId];
                var value = filter
                    ? BankManagerMath.Normalize(filter.values[parameterIndex], definition)
                    : NaN;
                if (!isFinite(value)) {
                    minimumDelta = 0;
                    maximumDelta = 0;
                    break;
                }
                minimumDelta = Math.max(minimumDelta, -value);
                maximumDelta = Math.min(maximumDelta, 1 - value);
            }
            outlet(2, "filter_limits", source.id, Number(filterId),
                parameterIndex,
                BankManagerMath.Denormalize(Math.max(0, sourceValue + minimumDelta), definition),
                BankManagerMath.Denormalize(Math.min(1, sourceValue + maximumDelta), definition));
        }
    }
};

BankManager.prototype.RefreshControlLinkSession = function() {
    var activeLinkId = this.ActiveLinkId(this.local);
    var activeMembers = activeLinkId
        ? this.LinkMemberIds(activeLinkId)
        : [];
    var activeBank = this.ActiveBank(this.local);
    var signature = (activeLinkId && activeMembers.length >= 2
        ? activeLinkId + ":" + activeMembers.join(",")
        : "unlinked") + ":" + (activeBank ? activeBank.id : 0);
    if (this.controlLinkSession === signature) return;
    this.controlLinkSession = signature;
    var color = activeLinkId ? this.LinkColor(activeLinkId) : null;
    outlet(2, "link_color", activeLinkId && color ? activeLinkId : "-",
        color ? color[0] : 0, color ? color[1] : 0,
        color ? color[2] : 0, color ? color[3] : 0);
    for (var device in this.local.processors) {
        if (!this.local.processors.hasOwnProperty(device)) continue;
        var processor = this.local.processors[device];
        var definitions = this.processorRanges[device] || {};
        var group = activeLinkId ? this.ProcessorLinkGroup(activeLinkId, device) : null;
        var memberIds = group ? Object.keys(group.members).sort() : [];
        var isLinked = activeLinkId && memberIds.length >= 2;
        for (var parameter in processor.values) {
            if (!processor.values.hasOwnProperty(parameter) || !definitions[parameter]) continue;
            var range = definitions[parameter];
            var effective = isLinked
                ? group.EffectiveRange(this.instanceId, parameter, range)
                : range;
            outlet(2, "processor_limits", device, parameter, effective.minimum, effective.maximum);
        }
    }
    this.PublishFilterLimits(activeLinkId, activeMembers.length >= 2);
    this.PublishLinkedFilterPreviews(activeLinkId, activeMembers.length >= 2);
    this.PublishLinkedDetectorPreviews(activeLinkId, activeMembers.length >= 2);
};

BankManager.prototype.PublishLinkedDetectorPreviews = function(linkId, isLinked) {
    var devices = ["compressor", "saturator"];
    for (var deviceIndex = 0; deviceIndex < devices.length; ++deviceIndex) {
        var device = devices[deviceIndex];
        outlet(2, "detector_link_preview", device, "-");
        if (!isLinked || !linkId) continue;
        var group = this.ProcessorLinkGroup(linkId, device);
        if (!group) continue;
        for (var sourceId in group.members) {
            if (!group.members.hasOwnProperty(sourceId) || sourceId === this.instanceId) continue;
            var values = group.members[sourceId].values;
            for (var filterId = 1; filterId <= 2; ++filterId) {
                var prefix = "detector." + filterId + ".";
                var bypass = Number(values[prefix + "bypass"]);
                var gain = Number(values[prefix + "gain"]);
                var frequency = Number(values[prefix + "frequency"]);
                var q = Number(values[prefix + "q"]);
                if (!isFinite(bypass) || !isFinite(gain) ||
                    !isFinite(frequency) || !isFinite(q)) continue;
                outlet(2, "detector_link_preview", device, linkId, sourceId,
                    filterId, bypass ? 0 : 1, gain, frequency, q);
            }
        }
    }
};

BankManager.prototype.PublishLinkedFilterPreviews = function(linkId, isLinked) {
    // Replace the complete preview set so removed peers cannot remain in SpectrumView.
    outlet(2, "eq_link_preview", "-");
    if (!isLinked || !linkId) {
        return;
    }
    var members = this.LinkMembers(linkId);
    for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
        var member = members[memberIndex];
        if (member.instance.id === this.instanceId) continue;
        for (var filterId in member.bank.filters) {
            if (!member.bank.filters.hasOwnProperty(filterId)) continue;
            var filter = member.bank.filters[filterId];
            var parameters = this.filterDefinitions[filterId];
            if (!parameters) continue;
            var frequency = 0;
            var gain = 0;
            var q = 0;
            for (var parameterIndex = 0;
                 parameterIndex < parameters.length;
                 ++parameterIndex) {
                var parameterName = parameters[parameterIndex].name;
                var value = Number(filter.values[parameterIndex]);
                if (!isFinite(value)) continue;
                if (parameterName === "freq" || parameterName === "pivot") frequency = value;
                else if (parameterName === "gain") gain = value;
                else if (parameterName === "q") q = value;
            }
            outlet(2, "eq_link_preview", linkId, member.instance.id,
                Number(filterId), filter.bypass ? 0 : 1, frequency, gain, q,
                this.filterTypes[filterId] || "peak");
        }
    }
};

BankManager.prototype.ParseDeviceSnapshot = function(values) {
    if (values.length !== 6 || String(values[0]) !== "snapshot" || String(values[3]) !== "device") return;
    if (!this.instanceId) return;
    this.PublishAnnouncement();
};

BankManager.prototype.HandleGlobal = function(name, values) {
    var shouldRedraw = name !== "link.filter_delta" &&
        name !== "link.processor_delta" &&
        name !== "link.state";
    if (name === "bank.query") {
        if (String(values[0]) !== this.instanceId) {
            this.lastAnnouncementState = "";
            this.PublishAnnouncement();
            this.PublishLinkedState();
        }
    } else if (name === "bank.announce") {
        this.ParseAnnouncement(values);
    } else if (name === "bank.reset_all") {
        if (values.length === 1) this.SendHostCommand("eq.reset_all", []);
    } else if (name === "link.state") {
        this.ApplyLinkState(values);
    } else if (name === "bank.leave") {
        this.RemovePeer(values);
    } else if (name === "link.assign") {
        this.ApplyLinkAssignment(values);
    } else if (name === "link.detach") {
        this.ApplyLinkDetachment(values);
    } else if (name === "link.operation") {
        this.ApplyLinkOperation(values);
    } else if (name === "link.filter_delta") {
        this.ApplyFilterDelta(values);
    } else if (name === "link.filter_bypass") {
        this.ApplyLinkBypass(values);
    } else if (name === "link.filter_reset") {
        this.ApplyLinkFilterReset(values);
    } else if (name === "link.processor_delta") {
        this.ApplyProcessorDelta(values);
    } else if (name === "link.processor_detector_reset") {
        this.ApplyProcessorDetectorReset(values);
    }
    if (shouldRedraw) mgraphics.redraw();
};

BankManager.prototype.ApplyProcessorDelta = function(values) {
    if (values.length !== 6) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var device = String(values[3]);
    var parameter = String(values[4]);
    var delta = Number(values[5]);
    if (sourceId === this.instanceId || !isFinite(revision) ||
        !isFinite(delta)) return;
    var group = this.ProcessorLinkGroup(linkId, device);
    if (!group) return;
    var range = this.processorRanges[device] &&
        this.processorRanges[device][parameter];
    if (!range) return;
    if (!this.AcceptIncomingLinkRevision(
        linkId, sourceId, revision)) return;
    group.ApplyDelta(sourceId, parameter, delta, false, range);
    var processor = this.local.processors[device];
    if (!processor || !this.HasLink(this.local, linkId)) return;
    this.PublishProcessorPreview(device, parameter, processor.values[parameter]);
    if (parameter.indexOf("detector.") === 0) {
        this.PublishLinkedDetectorPreviews(linkId, true);
    }
    this.SendProcessorValue(device, parameter, processor.values[parameter]);
};

BankManager.prototype.ApplyProcessorDetectorReset = function(values) {
    if (values.length !== 5) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var device = String(values[3]);
    var filterId = Number(values[4]);
    if (sourceId === this.instanceId ||
        (device !== "compressor" && device !== "saturator") ||
        !isFinite(filterId) || !this.AcceptIncomingOperationRevision(
            linkId, sourceId, revision)) return;
    if (!this.HasLink(this.local, linkId)) return;
    this.ResetDetectorModels(linkId, device, filterId);
    this.SendDetectorReset(device, filterId);
    this.PublishLinkedDetectorPreviews(linkId, true);
};

BankManager.prototype.RemovePeer = function(values) {
    if (values.length !== 1) return;
    var instanceId = String(values[0]);
    if (!instanceId || instanceId === this.instanceId) return;
    delete this.peers[instanceId];
    if (this.selection.focusedInstanceId === instanceId) {
        this.SetFocusedBank(this.local, this.local.selectedBankId);
    }
    this.RebuildProcessorLinkGroups();
    this.controlLinkSession = "";
    this.RefreshControlLinkSession();
};

BankManager.prototype.ApplyFilterDelta = function(values) {
    if (values.length !== 6) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var filterId = Number(values[3]);
    var parameterIndex = Number(values[4]);
    var delta = Number(values[5]);
    if (sourceId === this.instanceId || !isFinite(revision) ||
        !isFinite(filterId) || !isFinite(parameterIndex) || !isFinite(delta)) return;
    var bank = this.FindLocalLinkedBank(linkId);
    var filter = bank && bank.filters[filterId];
    if (!filter || !isFinite(filter.values[parameterIndex]) ||
        !this.AcceptIncomingLinkRevision(
            linkId, sourceId, revision)) return;
    var update = {
        linkId: linkId,
        filterId: filterId,
        parameterIndex: parameterIndex,
        delta: delta
    };
    this.ApplyFilterDeltaToModel(update, "");
    this.PublishEqPreview(bank.id, filterId, parameterIndex,
        filter.values[parameterIndex]);
    this.PublishLinkedFilterPreviews(linkId, true);
    this.SendHostCommand("eq.set_parameter_index", [
        bank.id, filterId, parameterIndex, filter.values[parameterIndex]
    ]);
};

BankManager.prototype.ApplyLinkBypass = function(values) {
    if (values.length !== 5) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var filterId = Number(values[3]);
    var bypass = Number(values[4]) ? 1 : 0;
    if (sourceId === this.instanceId || !isFinite(revision) ||
        !isFinite(filterId)) return;
    var localBank = this.FindLocalLinkedBank(linkId);
    if (!localBank || !this.AcceptIncomingLinkRevision(
        linkId, sourceId, revision)) return;
    var members = this.LinkMembers(linkId);
    for (var index = 0; index < members.length; index++) {
        var filter = members[index].bank.filters[filterId];
        if (filter) filter.bypass = bypass !== 0;
    }
    this.PublishLinkedFilterPreviews(linkId, true);
    this.SendHostCommand("eq.set_bypass", [localBank.id, filterId, bypass]);
};

BankManager.prototype.ExecuteFilterReset = function(bankId, filterId) {
    var bank = this.LocalBank(bankId);
    if (!bank || !isFinite(filterId)) return;
    if (!bank.linkId) {
        this.ResetFilterModel(bank, filterId);
        this.SendHostCommand("eq.reset_filter", [bank.id, filterId]);
        return;
    }
    this.ResetLinkedFilterModels(bank.linkId, filterId);
    this.PublishLinkedFilterPreviews(bank.linkId, true);
    this.SendHostCommand("eq.reset_filter", [bank.id, filterId]);
    outlet(1, "link.filter_reset", bank.linkId, this.instanceId,
        this.NextLinkRevision(bank.linkId), filterId);
};

BankManager.prototype.ApplyLinkFilterReset = function(values) {
    if (values.length !== 4) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var filterId = Number(values[3]);
    if (sourceId === this.instanceId || !isFinite(filterId) ||
        !this.AcceptIncomingOperationRevision(linkId, sourceId, revision)) return;
    var bank = this.FindLocalLinkedBank(linkId);
    if (!bank) return;
    this.ResetLinkedFilterModels(linkId, filterId);
    this.PublishLinkedFilterPreviews(linkId, true);
    this.SendHostCommand("eq.reset_filter", [bank.id, filterId]);
};

BankManager.prototype.ResetLinkedFilterModels = function(linkId, filterId) {
    var members = this.LinkMembers(linkId);
    for (var index = 0; index < members.length; ++index) {
        this.ResetFilterModel(members[index].bank, filterId);
    }
};

BankManager.prototype.ResetFilterModel = function(bank, filterId) {
    var filter = bank && bank.filters[filterId];
    var parameters = this.filterDefinitions[filterId];
    if (!filter || !parameters) return;
    var values = [];
    for (var index = 0; index < parameters.length; ++index) {
        values.push(Number(parameters[index].defaultValue));
    }
    filter.values = values;
    filter.bypass = Boolean(this.filterDefaultBypass[filterId]);
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
    this.viewModel.listView.SetItems(this.Rows());
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
    outlet(1, "bank.reset_all", this.instanceId);
    mgraphics.redraw();
};

BankManager.prototype.ClearAllConfirmationExpired = function() {
    this.clearAllConfirmationArmed = false;
    mgraphics.redraw();
};

BankManager.prototype.LinkGroupIndexAt = function(x, y, width, height) {
    var layout = new ButtonGroupLayout();
    return layout.IndexAt(
        this.LinkPanelRect(width, height),
        this.EditableLinkIds().length + 1,
        x,
        y,
        BankManagerButtonGroupOptions.links
    );
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

BankManager.prototype.CanAssignFocusedBankToLink = function(linkId) {
    var instance = this.FocusedInstance();
    var bank = this.FocusedBank();
    var nextLinkId = this.NormalizeLinkId(linkId);
    if (!instance || !bank || !this.CanChangeFocusedBankLink()) return false;
    if (!nextLinkId) return Boolean(bank.linkId);
    if (this.EditableLinkIds().indexOf(nextLinkId) < 0 || bank.linkId === nextLinkId) {
        return false;
    }
    for (var index = 0; index < instance.banks.length; ++index) {
        var candidate = instance.banks[index];
        if (candidate.id !== bank.id && candidate.linkId === nextLinkId) return false;
    }
    return true;
};

BankManager.prototype.IsActiveGroupMember = function(bank) {
    var linkId = this.FocusedLinkId();
    return Boolean(linkId && bank && bank.linkId === linkId);
};

BankManager.prototype.CanEditBankInActiveGroup = function(instance, bank) {
    var linkId = this.ActiveEditableLinkId();
    if (!linkId || !instance || !bank || bank.id < 2 || bank.id > 5) return false;
    if (bank.linkId === linkId) return true;
    if (bank.linkId) return false;
    for (var index = 0; index < instance.banks.length; ++index) {
        if (instance.banks[index].id !== bank.id &&
            instance.banks[index].linkId === linkId) return false;
    }
    return true;
};

BankManager.prototype.SetFocusedBankLink = function(linkId) {
    var instance = this.FocusedInstance();
    var bank = this.FocusedBank();
    var nextLinkId = this.NormalizeLinkId(linkId);
    if (!this.linkEditingEnabled || !instance || !bank ||
        !this.CanAssignFocusedBankToLink(nextLinkId)) return;
    if (instance.id === this.instanceId) {
        this.SendHostCommand("eq.set_link", [bank.id, nextLinkId || "-"]);
    } else if (nextLinkId) {
        outlet(1, "link.assign", nextLinkId, instance.id, bank.id);
    } else {
        outlet(1, "link.detach", bank.linkId, instance.id, bank.id);
    }
};

BankManager.prototype.ToggleBankInActiveGroup = function(instance, bank) {
    var linkId = this.ActiveEditableLinkId();
    if (!this.linkEditingEnabled || !linkId ||
        !this.CanEditBankInActiveGroup(instance, bank)) return;
    if (bank.linkId === linkId) {
        outlet(1, "link.detach", linkId, instance.id, bank.id);
    } else {
        outlet(1, "link.assign", linkId, instance.id, bank.id);
    }
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

BankManager.prototype.HandleProcessorDetectorReset = function(values) {
    this.linkTransport.HandleDetectorReset(values);
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
        ? "Local input: Host snapshots; eq_parameter_absolute_gesture, eq_parameter_absolute_preview, eq_filter_reset, processor_parameter_gesture"
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
function onclick(x, y) { bankManager.ui.Click(x, y); }
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
