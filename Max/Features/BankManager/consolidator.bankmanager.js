autowatch = 1;
inlets = 2;
outlets = 3;

// Inlet 0: Host EQ, processor, and device snapshots.
// Inlet 1: global bank/link messages: bank.query, bank.announce, bank.leave,
// bank.reset_all, link.assign, link.detach, link.operation, link.filter_delta,
// link.filter_bypass, link.state, and link.processor_delta.
// Outlet 0: Host commands: eq.select_bank, eq.join_banks, eq.commit_all,
// eq.reset_all, eq.set_link, gain.set_parameter, compressor.set_parameter,
// and saturator.set_parameter.
// Outlet 1: the complete global bank/link message set accepted by inlet 1.
// Outlet 2: link_color, processor_limits, processor_preview, eq_preview, and filter_limits UI state.
// Local gesture input also accepts eq_parameter_absolute_gesture
// <bankId> <filterId> <parameter> <absoluteValue> from SpectrumView.
// bank.announce: <instanceId> <trackName> <trackOrder> <revision> <selectedBank>
// <systemOccupied> <six bankId occupied linkId records>.
// link.assign|link.detach: <linkId> <instanceId> <bankId>.
// link.operation: <linkId> <sourceId> <revision> <join|commit|reset|bypass> <bypass|-1>.
// link.filter_delta: <linkId> <sourceId> <revision> <filterId> <parameterIndex> <normalizedDelta>.
// link.filter_bypass: <linkId> <sourceId> <revision> <filterId> <0|1>.
// link.processor_delta: <linkId> <sourceId> <revision> <device> <parameter> <normalizedDelta>.
// link.state: <linkId> <sourceId> <revision> <bankId> <filterCount>
// <filterId> <bypass> <valueCount> <values...>... <processorValueCount>
// <device> <parameter> <absoluteValue>...

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;
include("../../Shared/Runtime/LiveApiInitializer.js");
include("../../Shared/Runtime/ControlControllerBase.js");
include("../../Shared/Interface/BankManager/BankManagerOptions.js");
include("../../Shared/Interface/BankManager/BankManagerViewModel.js");
include("../../Shared/Interface/BankManager/BankManagerRenderer.js");
include("../../Shared/Interface/ButtonGroup/ButtonGroupLayout.js");
include("../../Shared/Configuration/FilterDefinitions.js");
var BankManagerVisualOptions = BankManagerOptions.geometry;
var BankManagerColors = BankManagerOptions.colors;
var bankGroupLayout = new ButtonGroupLayout();

function NormalizeParameter(value, range) {
    var number = Number(value);
    if (!range || !isFinite(number) || range.maximum === range.minimum) return NaN;
    if (range.logarithmic && range.minimum > 0 && number > 0) {
        return Math.log(number / range.minimum)
            / Math.log(range.maximum / range.minimum);
    }
    return (number - range.minimum) / (range.maximum - range.minimum);
}

function DenormalizeParameter(value, range) {
    var normalized = Math.max(0, Math.min(1, Number(value)));
    if (!range || !isFinite(normalized)) return NaN;
    if (range.logarithmic && range.minimum > 0) {
        return range.minimum
            * Math.pow(range.maximum / range.minimum, normalized);
    }
    return range.minimum + normalized * (range.maximum - range.minimum);
}

function BankSummary() {
    this.id = 0;
    this.occupied = false;
    this.linkId = "";
    this.filters = {};
}

function ProcessorSummary(id, label) {
    this.id = id;
    this.label = label;
    this.values = {};
}

function ProcessorLinkGroup(linkId, device) {
    this.linkId = linkId;
    this.device = device;
    this.members = {};
}

ProcessorLinkGroup.prototype.AddMember = function(instanceId, processor) {
    this.members[instanceId] = processor;
};

ProcessorLinkGroup.prototype.EffectiveRange = function(sourceId, parameter, range) {
    var source = this.members[sourceId];
    var sourceValue = source
        ? NormalizeParameter(source.values[parameter], range)
        : NaN;
    if (!range || !isFinite(sourceValue) || Object.keys(this.members).length < 2) return range;
    var minimumDelta = -Infinity;
    var maximumDelta = Infinity;
    for (var instanceId in this.members) {
        if (!this.members.hasOwnProperty(instanceId) || instanceId === sourceId) continue;
        var value = NormalizeParameter(this.members[instanceId].values[parameter], range);
        if (!isFinite(value)) {
            var lockedValue = DenormalizeParameter(sourceValue, range);
            return { minimum: lockedValue, maximum: lockedValue };
        }
        minimumDelta = Math.max(minimumDelta, -value);
        maximumDelta = Math.min(maximumDelta, 1 - value);
    }
    return {
        minimum: DenormalizeParameter(Math.max(0, sourceValue + minimumDelta), range),
        maximum: DenormalizeParameter(Math.min(1, sourceValue + maximumDelta), range)
    };
};

ProcessorLinkGroup.prototype.ApplyDelta = function(
    sourceId,
    parameter,
    delta,
    sourceAlreadyApplied,
    range
) {
    for (var instanceId in this.members) {
        if (!this.members.hasOwnProperty(instanceId) ||
            (sourceAlreadyApplied && instanceId === sourceId)) continue;
        var processor = this.members[instanceId];
        var normalized = NormalizeParameter(processor.values[parameter], range);
        if (isFinite(normalized)) {
            processor.values[parameter] = DenormalizeParameter(normalized + delta, range);
        }
    }
};

function InstanceSummary(id, label) {
    this.id = id;
    this.label = label;
    this.trackOrder = Infinity;
    this.revision = 0;
    this.selectedBankId = 1;
    this.systemBank = new BankSummary();
    this.banks = [];
    for (var bankId = 1; bankId <= 6; bankId++) {
        var bank = new BankSummary();
        bank.id = bankId;
        this.banks.push(bank);
    }
    this.processors = {
        compressor: new ProcessorSummary("compressor", "COMP"),
        saturator: new ProcessorSummary("saturator", "SAT"),
        input_gain: new ProcessorSummary("input_gain", "IN"),
        output_gain: new ProcessorSummary("output_gain", "OUT")
    };
}

function BankManager() {
    box.message("border", 0);
    ControlControllerBase.call(this, "bankmanager.ui", null, this);
    this.instanceId = "";
    this.local = new InstanceSummary("", "Consolidator");
    this.peers = {};
    this.focusedInstanceId = "";
    this.focusedBankId = 1;
    this.linkEditingEnabled = false;
    this.clearAllConfirmationArmed = false;
    this.clearAllConfirmationTask = new Task(this.ClearAllConfirmationExpired, this);
    this.linkRevision = 0;
    this.outgoingLinkRevisions = {};
    this.incomingLinkRevisions = {};
    this.filterDefinitions = {};
    this.processorRanges = {};
    this.LoadDefinitions();
    this.processorLinkGroups = {};
    this.controlLinkSession = "";
    this.viewModel = new BankManagerViewModel();
    this.renderer = new BankManagerRenderer();
    this.lastAnnouncementState = "";
    this.initializer = new LiveApiInitializer(
        this.TryInitialize, this, 50);
}

BankManager.prototype = Object.create(ControlControllerBase.prototype);
BankManager.prototype.constructor = BankManager;

BankManager.prototype.LoadDefinitions = function() {
    var eqDefinitions = FilterDefinitionCatalog.Eq();
    for (var filterId in eqDefinitions) {
        if (!eqDefinitions.hasOwnProperty(filterId)) continue;
        this.filterDefinitions[filterId] = eqDefinitions[filterId].parameters;
    }
    var processorDefinitions = FilterDefinitionCatalog.Processors();
    for (var device in processorDefinitions) {
        if (!processorDefinitions.hasOwnProperty(device)) continue;
        this.processorRanges[device] = {};
        var parameters = processorDefinitions[device].parameters;
        for (var index = 0; index < parameters.length; index++) {
            var parameter = parameters[index];
            this.processorRanges[device][parameter.name] = {
                minimum: parameter.minimum,
                maximum: parameter.maximum,
                logarithmic: parameter.logarithmic
            };
        }
    }
};

BankManager.prototype.Initialize = function() {
    this.initializer.Start();
};

BankManager.prototype.TryInitialize = function() {
    var identity = this.CurrentRuntimeIdentity();
    if (!identity) return false;
    this.instanceId = identity.id;
    this.local.id = this.instanceId;
    this.local.label = identity.trackName;
    this.local.trackOrder = identity.trackOrder;
    this.SetFocusedBank(this.local, this.local.selectedBankId);
    outlet(1, "bank.query", this.instanceId);
    this.PublishAnnouncement();
    this.PublishLinkedState();
    return true;
};

BankManager.prototype.CurrentRuntimeIdentity = function() {
    try {
        var device = new LiveAPI("this_device");
        var liveObjectId = Number(device.id);
        var parent = device.get("canonical_parent");
        var trackId = Number(parent[1]);
        if (liveObjectId <= 0 || trackId <= 0) return null;
        var track = new LiveAPI("id " + trackId);
        var trackName = String(track.get("name")[0] || "");
        var trackOrder = this.TrackOrder(trackId);
        if (!trackName || !isFinite(trackOrder)) return null;
        return {
            id: "live-device-" + String(liveObjectId),
            trackName: trackName,
            trackOrder: trackOrder
        };
    } catch (error) {
        return null;
    }
};

BankManager.prototype.TrackOrder = function(trackId) {
    var liveSet = new LiveAPI("live_set");
    var tracksCount = Number(liveSet.getcount("tracks"));
    if (isFinite(tracksCount) && tracksCount >= 0) {
        for (var trackIndex = 0; trackIndex < tracksCount; trackIndex++) {
            var track = new LiveAPI("live_set tracks " + trackIndex);
            if (Number(track.id) === trackId) return trackIndex;
        }
    }

    var returnCount = Number(liveSet.getcount("return_tracks"));
    if (isFinite(returnCount) && returnCount >= 0) {
        for (var returnIndex = 0; returnIndex < returnCount; returnIndex++) {
            var returnTrack = new LiveAPI("live_set return_tracks " + returnIndex);
            if (Number(returnTrack.id) === trackId) {
                return (isFinite(tracksCount) ? tracksCount : 0) + returnIndex;
            }
        }
    }

    var masterTrack = new LiveAPI("live_set master_track");
    if (Number(masterTrack.id) === trackId) {
        return (isFinite(tracksCount) ? tracksCount : 0)
            + (isFinite(returnCount) ? returnCount : 0);
    }
    return NaN;
};

BankManager.prototype.SendHostCommand = function(name, fields) {
    this.SendCommand(name, fields);
};

BankManager.prototype.ParseEqSnapshot = function(values) {
    if (values.length < 10 || String(values[0]) !== "snapshot" || Number(values[1]) !== 1 ||
        String(values[2]) !== "host" || String(values[3]) !== "eq") return false;
    var revision = Number(values[4]);
    var selected = Number(values[5]);
    var bankCount = Number(values[8]);
    if (!isFinite(revision) || selected < 1 || selected > 6 || bankCount !== 7) return false;
    var position = 9;
    var previousActiveLinkId = this.ActiveLinkId(this.local);
    var previousLinkIds = {};
    for (var previousIndex = 0; previousIndex < this.local.banks.length; previousIndex++) {
        var previousLinkId = this.local.banks[previousIndex].linkId;
        if (previousLinkId) previousLinkIds[previousIndex + 1] = previousLinkId;
    }
    var banks = [];
    var systemBank = new BankSummary();
    for (var index = 0; index < bankCount; index++) {
        if (position + 2 >= values.length) return false;
        var bankId = Number(values[position++]);
        var linkId = String(values[position++]);
        var filterCount = Number(values[position++]);
        if (bankId !== index || !isFinite(filterCount) || filterCount < 0) return false;
        var occupied = false;
        var filters = {};
        for (var filterIndex = 0; filterIndex < filterCount; filterIndex++) {
            if (position + 2 >= values.length) return false;
            var filterId = Number(values[position++]);
            var filterBypass = Number(values[position++]) !== 0;
            var valueCount = Number(values[position++]);
            if (!isFinite(valueCount) || valueCount < 0 || position + valueCount > values.length) return false;
            var filterValues = [];
            var parameters = this.filterDefinitions[filterId] || [];
            for (var valueIndex = 0; valueIndex < valueCount; valueIndex++) {
                var filterValue = Number(values[position + valueIndex]);
                filterValues.push(filterValue);
                if (parameters[valueIndex] && parameters[valueIndex].name === "gain" &&
                    Math.abs(filterValue) > 1.0e-12) occupied = true;
            }
            position += valueCount;
            filters[filterId] = { bypass: filterBypass, values: filterValues };
        }
        if (bankId === 0) {
            systemBank.occupied = occupied;
            systemBank.filters = filters;
        } else {
            var bank = new BankSummary();
            bank.id = bankId;
            bank.occupied = occupied;
            bank.linkId = linkId;
            bank.filters = filters;
            banks.push(bank);
        }
    }
    if (position !== values.length) return false;
    this.local.revision = revision;
    this.local.eqBypass = Number(values[6]) !== 0;
    this.local.selectedBankId = selected;
    this.local.systemBank = systemBank;
    this.local.banks = banks;
    if (!this.focusedInstanceId || this.focusedInstanceId === this.instanceId) {
        this.SetFocusedBank(this.local, selected);
    }
    var changedLinkIds = {};
    var linkTopologyChanged = false;
    for (var bankIndex = 0; bankIndex < banks.length; bankIndex++) {
        var previousLinkId = previousLinkIds[bankIndex + 1] || "";
        var currentLinkId = banks[bankIndex].linkId || "";
        if (previousLinkId === currentLinkId) continue;
        linkTopologyChanged = true;
        if (currentLinkId) changedLinkIds[currentLinkId] = true;
    }
    if (linkTopologyChanged) this.RebuildProcessorLinkGroups();
    else if (previousActiveLinkId !== this.ActiveLinkId(this.local)) {
        this.controlLinkSession = "";
        this.RefreshControlLinkSession();
    }
    this.PublishAnnouncement();
    this.PublishLinkedState(changedLinkIds);
    return true;
};

BankManager.prototype.ParseProcessorSnapshot = function(values) {
    if (values.length < 29 || String(values[3]) !== "processor") return;
    var count = values.length;
    var base = count - 29;
    var processors = {
        compressor: new ProcessorSummary("compressor", "COMP"),
        saturator: new ProcessorSummary("saturator", "SAT"),
        input_gain: new ProcessorSummary("input_gain", "IN"),
        output_gain: new ProcessorSummary("output_gain", "OUT")
    };
    processors.input_gain.values.gain = Number(values[base]);
    processors.compressor.values = {
        attack: Number(values[base + 2]),
        release: Number(values[base + 3]),
        threshold: Number(values[base + 4]),
        output: Number(values[base + 5]),
        mix: Number(values[base + 6])
    };
    processors.saturator.values = {
        saturation: Number(values[base + 17]),
        output: Number(values[base + 18])
    };
    processors.output_gain.values.gain = Number(values[base + 28]);
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
    return instance && instance.banks[instance.selectedBankId - 1]
        ? instance.banks[instance.selectedBankId - 1]
        : null;
};

BankManager.prototype.ActiveLinkId = function(instance) {
    var bank = this.ActiveBank(instance);
    return bank ? bank.linkId : "";
};

BankManager.prototype.FocusedInstance = function() {
    if (!this.focusedInstanceId || this.focusedInstanceId === this.instanceId) {
        return this.local;
    }
    return this.peers[this.focusedInstanceId] || this.local;
};

BankManager.prototype.FocusedBank = function() {
    var instance = this.FocusedInstance();
    return instance && instance.banks[this.focusedBankId - 1]
        ? instance.banks[this.focusedBankId - 1]
        : null;
};

BankManager.prototype.IsFocusedBank = function(instance, bank) {
    return Boolean(instance && bank &&
        instance.id === this.FocusedInstance().id &&
        bank.id === this.focusedBankId);
};

BankManager.prototype.SetFocusedBank = function(instance, bankId) {
    if (!instance || !isFinite(bankId) || bankId < 1 || bankId > 6) return;
    this.focusedInstanceId = instance.id;
    this.focusedBankId = bankId;
    if (!this.CanChangeFocusedBankLink()) this.linkEditingEnabled = false;
    this.controlLinkSession = "";
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
    } else {
        this.SendHostCommand(device + ".set_parameter", [parameter, value]);
    }
};

BankManager.prototype.NormalizeLinkId = function(value) {
    var linkId = String(value || "");
    return linkId === "-" ? "" : linkId;
};

BankManager.prototype.RebuildProcessorLinkGroups = function() {
    var groups = {};
    var rows = this.Rows();
    for (var rowIndex = 0; rowIndex < rows.length; rowIndex++) {
        var instance = rows[rowIndex];
        var links = {};
        for (var bankIndex = 0; bankIndex < instance.banks.length; bankIndex++) {
            var linkId = instance.banks[bankIndex].linkId;
            if (linkId) links[linkId] = true;
        }
        for (var linkId in links) {
            for (var device in instance.processors) {
                if (!instance.processors.hasOwnProperty(device)) continue;
                var key = linkId + ":" + device;
                if (!groups[key]) groups[key] = new ProcessorLinkGroup(linkId, device);
                groups[key].AddMember(instance.id, instance.processors[device]);
            }
        }
    }
    this.processorLinkGroups = groups;
    this.RefreshControlLinkSession();
};

BankManager.prototype.ProcessorLinkGroup = function(linkId, device) {
    return this.processorLinkGroups[linkId + ":" + device] || null;
};

BankManager.prototype.LinkMembers = function(linkId) {
    var members = [];
    var rows = this.Rows();
    for (var rowIndex = 0; rowIndex < rows.length; rowIndex++) {
        for (var bankIndex = 0; bankIndex < rows[rowIndex].banks.length; bankIndex++) {
            var bank = rows[rowIndex].banks[bankIndex];
            if (bank.linkId === linkId) {
                members.push({ instance: rows[rowIndex], bank: bank });
                break;
            }
        }
    }
    return members;
};

BankManager.prototype.LinkMemberIds = function(linkId) {
    var members = this.LinkMembers(linkId);
    var ids = [];
    for (var index = 0; index < members.length; index++) ids.push(members[index].instance.id);
    return ids.sort();
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
            var sourceValue = NormalizeParameter(
                sourceFilter.values[parameterIndex], definition);
            if (!isFinite(sourceValue)) continue;
            var minimumDelta = isLinked ? -Infinity : -sourceValue;
            var maximumDelta = isLinked ? Infinity : 1 - sourceValue;
            for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
                if (members[memberIndex].instance.id === this.instanceId) continue;
                var filter = members[memberIndex].bank.filters[filterId];
                var value = filter
                    ? NormalizeParameter(filter.values[parameterIndex], definition)
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
                DenormalizeParameter(Math.max(0, sourceValue + minimumDelta), definition),
                DenormalizeParameter(Math.min(1, sourceValue + maximumDelta), definition));
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
    var color = activeLinkId && activeMembers.length >= 2
        ? BankManagerColors.linkColors[Math.abs(this.Hash(activeLinkId)) % BankManagerColors.linkColors.length]
        : null;
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
};

BankManager.prototype.ParseDeviceSnapshot = function(values) {
    if (values.length !== 6 || String(values[0]) !== "snapshot" || String(values[3]) !== "device") return;
    if (!this.instanceId) return;
    this.PublishAnnouncement();
};

BankManager.prototype.HandleEqAbsoluteParameterGesture = function(values) {
    if (values.length !== 4) return;
    var bankId = Number(values[0]);
    var filterId = Number(values[1]);
    var parameterName = String(values[2]);
    var absolute = Number(values[3]);
    var parameters = this.filterDefinitions[filterId] || [];
    for (var parameterIndex = 0;
         parameterIndex < parameters.length;
         ++parameterIndex) {
        if (parameters[parameterIndex].name !== parameterName) continue;
        var normalized = NormalizeParameter(absolute, parameters[parameterIndex]);
        var bank = this.LocalBank(bankId);
        var filter = bank && bank.filters[filterId];
        if (!bank || !filter || !isFinite(normalized)) return;
        this.PublishEqPreview(bankId, filterId, parameterIndex, absolute);
        if (!bank.linkId) return;
        var previousNormalized = NormalizeParameter(
            filter.values[parameterIndex], parameters[parameterIndex]);
        var delta = normalized - previousNormalized;
        if (!isFinite(previousNormalized) || !isFinite(delta) || !delta) return;
        var update = {
            linkId: bank.linkId,
            bankId: bankId,
            filterId: filterId,
            parameterIndex: parameterIndex,
            delta: delta
        };
        filter.values[parameterIndex] = absolute;
        this.ApplyFilterDeltaToModel(update, this.instanceId);
        outlet(1, "link.filter_delta", update.linkId, this.instanceId,
            this.NextLinkRevision(update.linkId), filterId, parameterIndex, delta);
        return;
    }
};

BankManager.prototype.HandleEqAbsoluteParameterPreview = function(values) {
    if (values.length !== 4) return;
    var absoluteValue = Number(values[3]);
    if (!isFinite(absoluteValue)) return;
    var filterId = Number(values[1]);
    var parameterName = String(values[2]);
    var parameters = this.filterDefinitions[filterId] || [];
    for (var parameterIndex = 0;
         parameterIndex < parameters.length;
         ++parameterIndex) {
        if (parameters[parameterIndex].name !== parameterName) continue;
        this.PublishEqPreview(values[0], filterId, parameterIndex, absoluteValue);
        return;
    }
};

BankManager.prototype.HandleProcessorParameterGesture = function(values) {
    if (values.length !== 3) return;
    var device = String(values[0]);
    var parameter = String(values[1]);
    var normalized = Number(values[2]);
    var processor = this.local.processors[device];
    var range = this.processorRanges[device] &&
        this.processorRanges[device][parameter];
    var linkId = this.ActiveLinkId(this.local);
    var group = linkId ? this.ProcessorLinkGroup(linkId, device) : null;
    if (!processor || !range || !group || !isFinite(normalized)) return;

    var previousNormalized = NormalizeParameter(
        processor.values[parameter], range);
    var delta = normalized - previousNormalized;
    if (!isFinite(previousNormalized) || !isFinite(delta) || !delta) return;

    processor.values[parameter] = DenormalizeParameter(normalized, range);
    group.ApplyDelta(this.instanceId, parameter, delta, true, range);
    this.PublishProcessorPreview(device, parameter, processor.values[parameter]);
    outlet(1, "link.processor_delta", linkId, this.instanceId,
        this.NextLinkRevision(linkId), device, parameter, delta);
};

BankManager.prototype.NextLinkRevision = function(linkId) {
    var next = (this.outgoingLinkRevisions[linkId] || 0) + 1;
    this.outgoingLinkRevisions[linkId] = next;
    return next;
};

BankManager.prototype.PublishEqPreview = function(
    bankId,
    filterId,
    parameterIndex,
    absoluteValue
) {
    outlet(2, "eq_preview", Number(bankId), Number(filterId),
        Number(parameterIndex), Number(absoluteValue));
};

BankManager.prototype.PublishProcessorPreview = function(
    device,
    parameter,
    absoluteValue
) {
    outlet(2, "processor_preview", String(device), String(parameter),
        Number(absoluteValue));
};

BankManager.prototype.AcceptIncomingLinkRevision = function(
    linkId,
    sourceId,
    revision
) {
    if (!linkId || !sourceId || !isFinite(revision)) return false;
    var key = linkId + ":" + sourceId;
    if (revision <= (this.incomingLinkRevisions[key] || 0)) return false;
    this.incomingLinkRevisions[key] = revision;
    return true;
};

BankManager.prototype.ApplyFilterDeltaToModel = function(update, skipInstanceId) {
    var members = this.LinkMembers(update.linkId);
    var definition = (this.filterDefinitions[update.filterId] || [])[update.parameterIndex];
    for (var index = 0; index < members.length; index++) {
        if (members[index].instance.id === skipInstanceId) continue;
        var filter = members[index].bank.filters[update.filterId];
        var normalized = filter
            ? NormalizeParameter(filter.values[update.parameterIndex], definition)
            : NaN;
        if (isFinite(normalized)) {
            filter.values[update.parameterIndex] = DenormalizeParameter(
                normalized + update.delta,
                definition);
        }
    }
};

BankManager.prototype.PublishLinkBypass = function(linkId, filterId, bypass) {
    var members = this.LinkMembers(linkId);
    for (var index = 0; index < members.length; index++) {
        var filter = members[index].bank.filters[filterId];
        if (filter) filter.bypass = bypass !== 0;
    }
    outlet(1, "link.filter_bypass", linkId, this.instanceId, this.NextLinkRevision(linkId),
        filterId, bypass);
};

BankManager.prototype.PublishAnnouncement = function() {
    if (!this.instanceId || this.local.banks.length !== 6) return false;
    var stateParts = [this.instanceId, this.local.label, this.local.trackOrder,
        this.local.selectedBankId,
        this.local.systemBank.occupied ? 1 : 0];
    for (var stateIndex = 0; stateIndex < this.local.banks.length; stateIndex++) {
        stateParts.push(this.local.banks[stateIndex].occupied ? 1 : 0);
        stateParts.push(this.local.banks[stateIndex].linkId || "-");
    }
    var state = stateParts.join("|");
    if (state === this.lastAnnouncementState) return false;

    var fields = [this.instanceId, this.local.label, this.local.trackOrder,
        this.local.revision, this.local.selectedBankId,
        this.local.systemBank.occupied ? 1 : 0];
    for (var bankIndex = 0; bankIndex < this.local.banks.length; bankIndex++) {
        var bank = this.local.banks[bankIndex];
        fields.push(bank.id, bank.occupied ? 1 : 0, bank.linkId || "-");
    }
    this.lastAnnouncementState = state;
    outlet(1, "bank.announce", fields);
    return true;
};

BankManager.prototype.PublishLinkedState = function(linkIds) {
    for (var bankIndex = 0; bankIndex < this.local.banks.length; bankIndex++) {
        var bank = this.local.banks[bankIndex];
        if (!bank.linkId || (linkIds && !linkIds[bank.linkId])) continue;
        var fields = [bank.linkId, this.instanceId, this.local.revision, bank.id];
        var filters = [];
        for (var filterId in bank.filters) {
            if (!bank.filters.hasOwnProperty(filterId)) continue;
            var filter = bank.filters[filterId];
            filters.push({ id: Number(filterId), bypass: filter.bypass, values: filter.values });
        }
        fields.push(filters.length);
        for (var filterIndex = 0; filterIndex < filters.length; filterIndex++) {
            var filterState = filters[filterIndex];
            fields.push(filterState.id, filterState.bypass ? 1 : 0, filterState.values.length);
            for (var valueIndex = 0; valueIndex < filterState.values.length; valueIndex++) {
                fields.push(filterState.values[valueIndex]);
            }
        }
        var processorValues = [];
        for (var device in this.local.processors) {
            if (!this.local.processors.hasOwnProperty(device)) continue;
            var processor = this.local.processors[device];
            for (var parameter in processor.values) {
                if (!processor.values.hasOwnProperty(parameter) ||
                    !isFinite(processor.values[parameter])) continue;
                processorValues.push({
                    device: device,
                    parameter: parameter,
                    value: processor.values[parameter]
                });
            }
        }
        fields.push(processorValues.length);
        for (var processorIndex = 0; processorIndex < processorValues.length; processorIndex++) {
            var processorState = processorValues[processorIndex];
            fields.push(processorState.device, processorState.parameter, processorState.value);
        }
        outlet(1, "link.state", fields);
    }
};

BankManager.prototype.ParseAnnouncement = function(values) {
    if (values.length !== 24) return;
    var instanceId = String(values[0]);
    if (!instanceId || instanceId === this.instanceId) return;
    var trackOrder = Number(values[2]);
    var revision = Number(values[3]);
    var selected = Number(values[4]);
    if (!isFinite(trackOrder) || !isFinite(revision) || selected < 1 || selected > 6) return;
    var peer = this.peers[instanceId] || new InstanceSummary(instanceId, String(values[1]));
    if (revision < peer.revision) return;
    peer.label = String(values[1]);
    peer.trackOrder = trackOrder;
    peer.revision = revision;
    peer.selectedBankId = selected;
    peer.systemBank.occupied = Number(values[5]) !== 0;
    var position = 6;
    var linkTopologyChanged = false;
    for (var index = 0; index < 6; index++) {
        var bankId = Number(values[position++]);
        var occupied = Number(values[position++]) !== 0;
        var linkId = this.NormalizeLinkId(values[position++]);
        if (bankId !== index + 1) return;
        if (peer.banks[index].linkId !== linkId) {
            peer.banks[index].filters = {};
            linkTopologyChanged = true;
        }
        peer.banks[index].occupied = occupied;
        peer.banks[index].linkId = linkId;
    }
    this.peers[instanceId] = peer;
    if (linkTopologyChanged) this.RebuildProcessorLinkGroups();
};

BankManager.prototype.ApplyLinkState = function(values) {
    if (values.length < 6) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var bankId = Number(values[3]);
    var filterCount = Number(values[4]);
    var peer = this.peers[sourceId];
    if (!peer || sourceId === this.instanceId ||
        !isFinite(revision) || !isFinite(bankId) ||
        !isFinite(filterCount) || filterCount < 0) return;
    var bank = peer.banks[bankId - 1];
    if (!bank || bank.linkId !== linkId) return;
    var position = 5;
    var filters = {};
    for (var filterIndex = 0; filterIndex < filterCount; filterIndex++) {
        if (position + 2 >= values.length) return;
        var filterId = Number(values[position++]);
        var bypass = Number(values[position++]) !== 0;
        var valueCount = Number(values[position++]);
        if (!isFinite(filterId) || !isFinite(valueCount) || valueCount < 0 ||
            position + valueCount > values.length) return;
        var filterValues = [];
        for (var valueIndex = 0; valueIndex < valueCount; valueIndex++) {
            var value = Number(values[position++]);
            if (!isFinite(value)) return;
            filterValues.push(value);
        }
        filters[filterId] = { bypass: bypass, values: filterValues };
    }
    if (position >= values.length) return;
    var processorValueCount = Number(values[position++]);
    if (!isFinite(processorValueCount) || processorValueCount < 0 ||
        values.length !== position + processorValueCount * 3) return;
    var processors = {};
    for (var processorIndex = 0; processorIndex < processorValueCount; processorIndex++) {
        var device = String(values[position++]);
        var parameter = String(values[position++]);
        var value = Number(values[position++]);
        if (!isFinite(value) || !peer.processors[device]) return;
        if (!processors[device]) processors[device] = {};
        processors[device][parameter] = value;
    }
    bank.filters = filters;
    for (var processorDevice in processors) {
        if (!processors.hasOwnProperty(processorDevice)) continue;
        var processor = peer.processors[processorDevice];
        for (var processorParameter in processors[processorDevice]) {
            processor.values[processorParameter] = processors[processorDevice][processorParameter];
        }
    }
    if (this.ActiveLinkId(this.local) === linkId) {
        this.controlLinkSession = "";
        this.RefreshControlLinkSession();
    }
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
    } else if (name === "link.processor_delta") {
        this.ApplyProcessorDelta(values);
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
    if (this.ActiveLinkId(this.local) !== linkId) {
        this.controlLinkSession = "";
        this.RefreshControlLinkSession();
    }
    var processor = this.local.processors[device];
    if (!processor || !this.HasLink(this.local, linkId)) return;
    this.PublishProcessorPreview(device, parameter, processor.values[parameter]);
    this.SendProcessorValue(device, parameter, processor.values[parameter]);
};

BankManager.prototype.RemovePeer = function(values) {
    if (values.length !== 1) return;
    var instanceId = String(values[0]);
    if (!instanceId || instanceId === this.instanceId) return;
    delete this.peers[instanceId];
    if (this.focusedInstanceId === instanceId) {
        this.SetFocusedBank(this.local, this.local.selectedBankId);
    }
    this.RebuildProcessorLinkGroups();
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
    this.SendHostCommand("eq.set_bypass", [localBank.id, filterId, bypass]);
};

BankManager.prototype.FindLocalLinkedBank = function(linkId) {
    var active = this.ActiveBank(this.local);
    if (active && active.linkId === linkId) return active;
    for (var index = 0; index < this.local.banks.length; index++) {
        if (this.local.banks[index].linkId === linkId) {
            return this.local.banks[index];
        }
    }
    return null;
};

BankManager.prototype.LocalBank = function(bankId) {
    return this.local.banks[bankId - 1] || null;
};

BankManager.prototype.Rows = function() {
    var rows = [this.local].concat(Object.keys(this.peers).map(function(id) {
        return this.peers[id];
    }, this));
    rows.sort(function(left, right) {
        if (left.trackOrder !== right.trackOrder) {
            return left.trackOrder - right.trackOrder;
        }
        return left.id < right.id ? -1 : (left.id > right.id ? 1 : 0);
    });
    return rows;
};

BankManager.prototype.BankStartX = function(width) {
    var options = BankManagerVisualOptions;
    return Math.max(
        options.padding,
        width - options.linkPanelWidth - this.BankColumnWidth(width) - options.columnGap - options.padding
    );
};

BankManager.prototype.BankColumnWidth = function(width) {
    var options = BankManagerVisualOptions;
    return Math.min(
        options.bankColumnWidth,
        Math.max(1, width - options.padding * 2 - options.linkPanelWidth - options.columnGap)
    );
};

BankManager.prototype.Hash = function(value) {
    var hash = 0;
    for (var index = 0; index < value.length; index++) hash = ((hash << 5) - hash) + value.charCodeAt(index);
    return hash;
};

BankManager.prototype.ContentHeight = function() {
    return Math.max(0, mgraphics.size[1] - BankManagerVisualOptions.padding * 2);
};

BankManager.prototype.MaximumScrollOffset = function() {
    this.viewModel.listView.SetItems(this.Rows());
    return this.viewModel.listView.MaximumScrollOffset(
        this.ContentHeight(), BankManagerVisualOptions.rowHeight);
};

BankManager.prototype.Scroll = function(delta) {
    var step = Number(delta);
    if (!isFinite(step) || step === 0) return;
    this.viewModel.listView.SetItems(this.Rows());
    this.viewModel.listView.Scroll(
        step,
        BankManagerVisualOptions.rowHeight,
        this.ContentHeight()
    );
    mgraphics.redraw();
};

BankManager.prototype.EditableLinkIds = function() {
    var ids = [];
    for (var index = 1; index <= BankManagerVisualOptions.linkGroupCount; ++index) {
        ids.push("group." + String(index));
    }
    return ids;
};

BankManager.prototype.LinkColor = function(linkId) {
    return BankManagerColors.linkColors[
        Math.abs(this.Hash(String(linkId))) % BankManagerColors.linkColors.length
    ];
};

BankManager.prototype.LinkPanelRect = function(width, height) {
    var options = BankManagerVisualOptions;
    return {
        x: Math.max(0, width - options.linkPanelWidth),
        y: options.padding + options.linkEditHeight + options.clearAllHeight + options.linkPanelGap * 2,
        width: options.linkPanelWidth,
        height: Math.max(1, height - options.padding * 2
            - options.linkEditHeight - options.clearAllHeight - options.linkPanelGap * 2)
    };
};

BankManager.prototype.ClearAllRect = function(width) {
    var options = BankManagerVisualOptions;
    return {
        x: Math.max(0, width - options.linkPanelWidth),
        y: options.padding + options.linkEditHeight + options.linkPanelGap,
        width: options.linkPanelWidth,
        height: options.clearAllHeight
    };
};

BankManager.prototype.LinkEditRect = function(width) {
    var options = BankManagerVisualOptions;
    return {
        x: Math.max(0, width - options.linkPanelWidth),
        y: options.padding,
        width: options.linkPanelWidth,
        height: options.linkEditHeight
    };
};

BankManager.prototype.IsPointInRect = function(x, y, rect) {
    return x >= rect.x && x <= rect.x + rect.width &&
        y >= rect.y && y <= rect.y + rect.height;
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
    if (values.length !== 3) return;
    var linkId = String(values[0]);
    var instanceId = String(values[1]);
    var bankId = Number(values[2]);
    if (instanceId !== this.instanceId || this.EditableLinkIds().indexOf(linkId) < 0 ||
        bankId < 2 || bankId > 5) return;
    this.SendHostCommand("eq.set_link", [bankId, linkId]);
};

BankManager.prototype.ApplyLinkDetachment = function(values) {
    if (values.length !== 3) return;
    var linkId = String(values[0]);
    var instanceId = String(values[1]);
    var bankId = Number(values[2]);
    if (instanceId !== this.instanceId || bankId < 2 || bankId > 5) return;
    var bank = this.LocalBank(bankId);
    if (bank && bank.linkId === linkId) this.SendHostCommand("eq.set_link", [bankId, "-"]);
};

BankManager.prototype.ExecuteOperation = function(action, bypass) {
    var activeBank = this.ActiveBank(this.local);
    if (!activeBank) return;
    var linkId = activeBank.linkId;
    if (linkId) {
        outlet(1, "link.operation", linkId, this.instanceId,
            this.NextLinkRevision(linkId), action, bypass === undefined ? -1 : bypass);
        return;
    }
    this.ApplyOperation(action, activeBank.id, bypass);
};

BankManager.prototype.ApplyOperation = function(action, bankId, bypass) {
    if (action === "join") {
        this.SendHostCommand("eq.join_banks", [1, bankId]);
    } else if (action === "commit") {
        this.SendHostCommand("eq.commit_all", []);
    } else if (action === "reset") {
        this.SendHostCommand("eq.reset", [bankId]);
    } else if (action === "bypass") {
        this.SendHostCommand("eq.set_chain_bypass", [Number(bypass) !== 0 ? 1 : 0]);
    }
};

BankManager.prototype.ApplyLinkOperation = function(values) {
    if (values.length !== 5) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var action = String(values[3]);
    var bypass = Number(values[4]);
    if (!isFinite(revision) || !this.AcceptIncomingLinkRevision(linkId, sourceId, revision)) return;
    var bank = this.FindLocalLinkedBank(linkId);
    if (bank) this.ApplyOperation(action, bank.id, bypass);
};

BankManager.prototype.Paint = function() {
    this.renderer.Paint(this, mgraphics.size[0], mgraphics.size[1]);
};

BankManager.prototype.Click = function(x, y, ctrl, cmd, shift) {
    var options = BankManagerVisualOptions;
    if (this.IsPointInRect(x, y, this.LinkEditRect(mgraphics.size[0]))) {
        this.ToggleLinkEditing();
        return;
    }
    if (this.IsPointInRect(x, y, this.ClearAllRect(mgraphics.size[0]))) {
        this.ClearAllEqBanks();
        return;
    }
    var groupIndex = this.LinkGroupIndexAt(x, y, mgraphics.size[0], mgraphics.size[1]);
    if (groupIndex >= 0) {
        this.SetFocusedBankLink(
            groupIndex === 0 ? "" : this.EditableLinkIds()[groupIndex - 1]);
        return;
    }
    var rows = this.Rows();
    var contentHeight = this.ContentHeight();
    var rowIndex = Math.floor((y - options.padding + this.viewModel.listView.scrollOffset) / options.rowHeight);
    if (y >= options.padding && y < contentHeight + options.padding && rowIndex >= 0 && rowIndex < rows.length) {
        var instance = rows[rowIndex];
        if (x < this.BankStartX(mgraphics.size[0])) {
            this.SetFocusedBank(instance, instance.selectedBankId);
            mgraphics.redraw();
            return;
        }
        var displayedBanks = [rows[rowIndex].systemBank].concat(rows[rowIndex].banks);
        var squareY = Math.floor(options.padding + rowIndex * options.rowHeight
            - this.viewModel.listView.scrollOffset
            + (options.rowHeight - options.squareSize) * 0.5);
        var bankIndex = bankGroupLayout.IndexAt(
            {
                x: this.BankStartX(mgraphics.size[0]),
                y: squareY,
                width: this.BankColumnWidth(mgraphics.size[0]),
                height: options.squareSize
            },
            displayedBanks.length,
            x,
            y,
            BankManagerButtonGroupOptions.banks
        );
        if (bankIndex >= 0 && bankIndex < displayedBanks.length) {
            var bank = displayedBanks[bankIndex];
            if (bank.id === 0) return;
            if (instance.id === this.instanceId) {
                this.SetFocusedBank(instance, bank.id);
                this.SendHostCommand("eq.select_bank", [bank.id]);
            } else {
                if (this.linkEditingEnabled && this.ActiveEditableLinkId()) {
                    this.ToggleBankInActiveGroup(instance, bank);
                } else {
                    this.SetFocusedBank(instance, bank.id);
                }
            }
            mgraphics.redraw();
            return;
        }
    }
};

var bankManager = new BankManager();

function inletassist(index) {
    assist(index === 0
        ? "Local input: Host snapshots; eq_parameter_absolute_gesture, eq_parameter_absolute_preview, processor_parameter_gesture"
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
function paint() { bankManager.Paint(); }
function onclick(x, y, button, cmd, shift, capslock, option, ctrl) { bankManager.Click(x, y, ctrl, cmd, shift); }
function onwheel(x, y, scrollx, scrolly, cmd, shift, capslock, option, ctrl) { bankManager.Scroll(scrolly); }
function snapshot() {
    if (inlet === 0) {
        var values = ["snapshot"].concat(arrayfromargs(arguments));
        if (String(values[3]) === "eq") bankManager.ParseEqSnapshot(values);
        else if (String(values[3]) === "processor") bankManager.ParseProcessorSnapshot(values);
        else if (String(values[3]) === "device") bankManager.ParseDeviceSnapshot(values);
        mgraphics.redraw();
    }
}
function event() {}
function eq_parameter_absolute_gesture() {
    if (inlet === 0) {
        bankManager.HandleEqAbsoluteParameterGesture(
            arrayfromargs(arguments));
    }
}
function eq_parameter_absolute_preview() {
    if (inlet === 0) {
        bankManager.HandleEqAbsoluteParameterPreview(
            arrayfromargs(arguments));
    }
}
function processor_parameter_gesture() {
    if (inlet === 0) {
        bankManager.HandleProcessorParameterGesture(arrayfromargs(arguments));
    }
}
function bank_action(action, value) {
    if (inlet !== 0) return;
    var name = String(action);
    if (name !== "join" && name !== "commit" && name !== "reset" && name !== "bypass") return;
    bankManager.ExecuteOperation(name, value);
}
function anything() {
    var values = arrayfromargs(arguments);
    if (inlet === 0 && messagename === "snapshot") {
        var snapshotValues = ["snapshot"].concat(values);
        if (String(snapshotValues[3]) === "eq") bankManager.ParseEqSnapshot(snapshotValues);
        else if (String(snapshotValues[3]) === "processor") bankManager.ParseProcessorSnapshot(snapshotValues);
        else if (String(snapshotValues[3]) === "device") bankManager.ParseDeviceSnapshot(snapshotValues);
    } else if (inlet === 0 &&
        messagename === "eq_parameter_absolute_gesture") {
        bankManager.HandleEqAbsoluteParameterGesture(values);
    } else if (inlet === 0 &&
        messagename === "eq_parameter_absolute_preview") {
        bankManager.HandleEqAbsoluteParameterPreview(values);
    } else if (inlet === 0 && messagename === "processor_parameter_gesture") {
        bankManager.HandleProcessorParameterGesture(values);
    } else if (inlet === 0 && messagename === "bank.action") {
        bankManager.ExecuteOperation(String(values[0]), values[1]);
    } else if (inlet === 1) {
        bankManager.HandleGlobal(messagename, values);
    }
}
function list() {
    var values = arrayfromargs(arguments);
    if (inlet === 0 && values.length && String(values[0]) === "snapshot") {
        if (String(values[3]) === "eq") bankManager.ParseEqSnapshot(values);
        else if (String(values[3]) === "processor") bankManager.ParseProcessorSnapshot(values);
        else if (String(values[3]) === "device") bankManager.ParseDeviceSnapshot(values);
    }
}
function leave() {
    if (bankManager.instanceId) outlet(1, "bank.leave", bankManager.instanceId);
}
