autowatch = 1;
inlets = 2;
outlets = 3;

// Inlet 0: Host definitions, processor_definitions, EQ, processor, and device snapshots.
// Inlet 1: global bank/link messages: bank.query, bank.announce, bank.leave,
// link.create, link.remove, link.join, link.filter_delta, link.filter_bypass,
// link.filter_reset, link.bank_reset, link.filter_state,
// link.processor_state, link.state_end, and link.processor_delta.
// Outlet 0: Host commands: eq.select_bank, eq.join_banks, eq.commit_hidden,
// eq.set_link, gain.set_parameter, compressor.set_parameter,
// and saturator.set_parameter.
// Outlet 1: the complete global bank/link message set accepted by inlet 1.
// Outlet 2: link_color, processor_limits, and filter_limits UI state.
// Local gesture input also accepts eq_parameter_absolute_gesture
// <bankId> <filterId> <parameter> <absoluteValue> from SpectrumView.
// bank.announce: <instanceId> <label> <revision> <selectedBank>
// <systemOccupied> <six bankId occupied linkId records>.
// link.create: <linkId> <colorIndex> <count> <instanceId> <bank:N>...
// link.filter_delta: <linkId> <sourceId> <revision> <filterId> <parameterIndex> <normalizedDelta>.
// link.filter_bypass: <linkId> <sourceId> <revision> <filterId> <0|1>.
// link.processor_delta: <linkId> <sourceId> <revision> <device> <parameter> <normalizedDelta>.

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;
include("../Shared/JS/LinkColors.js");
include("../Shared/JS/LiveApiInitializer.js");
include("JS/BankManagerTheme.js");
var BankManagerVisualOptions = BankManagerTheme.geometry;
var BankManagerColors = BankManagerTheme.colors;

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
    this.requestId = 0;
    this.instanceId = "";
    this.local = new InstanceSummary("", "Consolidator");
    this.peers = {};
    this.linkSelection = {};
    this.joinSelection = {};
    this.linkRevision = 0;
    this.outgoingLinkRevisions = {};
    this.incomingLinkRevisions = {};
    this.filterDefinitions = {};
    this.processorRanges = {};
    this.processorLinkGroups = {};
    this.controlLinkSession = "";
    this.scrollOffset = 0;
    this.lastAnnouncementState = "";
    this.initializer = new LiveApiInitializer(
        this.TryInitialize, this, 50);
}

BankManager.prototype.Initialize = function() {
    this.initializer.Start();
};

BankManager.prototype.TryInitialize = function() {
    var instanceId = this.CurrentRuntimeInstanceId();
    if (!instanceId) return false;
    this.instanceId = instanceId;
    this.local.id = this.instanceId;
    this.local.label = this.CurrentLabel();
    outlet(1, "bank.query", this.instanceId);
    this.PublishAnnouncement();
    return true;
};

BankManager.prototype.CurrentRuntimeInstanceId = function() {
    try {
        var device = new LiveAPI("this_device");
        var liveObjectId = Number(device.id);
        return liveObjectId > 0 ? "live-device-" + String(liveObjectId) : "";
    } catch (error) {
        return "";
    }
};

BankManager.prototype.CurrentLabel = function() {
    try {
        var device = new LiveAPI("this_device");
        var parentId = Number(device.get("canonical_parent")[1]) || 0;
        var parent = parentId > 0 ? new LiveAPI("id " + parentId) : null;
        var parentName = parent ? String(parent.get("name")[0] || "Track") : "Track";
        var deviceName = String(device.get("name")[0] || "Consolidator");
        return parentName + " / " + deviceName;
    } catch (error) {
        return "Consolidator";
    }
};

BankManager.prototype.SendHostCommand = function(name, fields) {
    this.requestId += 1;
    outlet(0, "command", [1, "bankmanager.ui", this.requestId, name].concat(fields || []));
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
    this.local.selectedBankId = selected;
    this.local.systemBank = systemBank;
    this.local.banks = banks;
    if (previousActiveLinkId !== this.ActiveLinkId(this.local)) {
        this.RebuildProcessorLinkGroups();
    }
    this.PublishAnnouncement();
    return true;
};

BankManager.prototype.ParseDefinitions = function(values) {
    if (values.length < 6 || String(values[0]) !== "snapshot" || String(values[3]) !== "definitions") return;
    var count = Number(values[5]);
    var position = 6;
    for (var index = 0; index < count; index++) {
        if (position + 3 >= values.length) return;
        var filterId = Number(values[position++]);
        position++;
        position++;
        var parameterCount = Number(values[position++]);
        var parameters = [];
        for (var parameterIndex = 0; parameterIndex < parameterCount; parameterIndex++) {
            if (position + 4 >= values.length) return;
            parameters.push({
                name: String(values[position++]),
                minimum: Number(values[position++]),
                maximum: Number(values[position++]),
                logarithmic: Number(values[position++]) === 1
            });
            position += 1;
        }
        this.filterDefinitions[filterId] = parameters;
    }
    this.controlLinkSession = "";
    this.RefreshControlLinkSession();
};

BankManager.prototype.ParseProcessorDefinitions = function(values) {
    if (values.length < 6 || String(values[3]) !== "processor_definitions") return;
    var count = Number(values[5]);
    var position = 6;
    this.processorRanges = {};
    for (var index = 0; index < count; index++) {
        var device = String(values[position++]);
        var parameterCount = Number(values[position++]);
        this.processorRanges[device] = {};
        for (var parameterIndex = 0; parameterIndex < parameterCount; parameterIndex++) {
            var name = String(values[position++]);
            var minimum = Number(values[position++]);
            var maximum = Number(values[position++]);
            var logarithmic = Number(values[position++]) === 1;
            position += 1;
            this.processorRanges[device][name] = {
                minimum: minimum,
                maximum: maximum,
                logarithmic: logarithmic
            };
        }
    }
    this.controlLinkSession = "";
    this.RefreshControlLinkSession();
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
        if (!this.PublishAnnouncement()) this.PublishLinkedState();
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
    var members = isLinked ? this.LinkMembers(linkId) : [];
    for (var filterId in this.filterDefinitions) {
        if (!this.filterDefinitions.hasOwnProperty(filterId)) continue;
        var parameters = this.filterDefinitions[filterId];
        for (var parameterIndex = 0; parameterIndex < parameters.length; parameterIndex++) {
            var definition = parameters[parameterIndex];
            var minimum = definition.minimum;
            var maximum = definition.maximum;
            var sourceFilter = source && source.filters[filterId];
            var sourceValue = sourceFilter
                ? NormalizeParameter(sourceFilter.values[parameterIndex], definition)
                : NaN;
            if (isLinked && isFinite(sourceValue)) {
                var minimumDelta = -Infinity;
                var maximumDelta = Infinity;
                for (var memberIndex = 0; memberIndex < members.length; memberIndex++) {
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
                minimum = DenormalizeParameter(
                    Math.max(0, sourceValue + minimumDelta),
                    definition);
                maximum = DenormalizeParameter(
                    Math.min(1, sourceValue + maximumDelta),
                    definition);
            }
            outlet(2, "filter_limits", Number(filterId), parameterIndex,
                minimum, maximum);
        }
    }
};

BankManager.prototype.RefreshControlLinkSession = function() {
    var activeLinkId = this.ActiveLinkId(this.local);
    var activeMembers = activeLinkId
        ? this.LinkMemberIds(activeLinkId)
        : [];
    var signature = activeLinkId && activeMembers.length >= 2
        ? activeLinkId + ":" + activeMembers.join(",")
        : "unlinked";
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

BankManager.prototype.HandleEqParameterGesture = function(values) {
    if (values.length !== 4) return;
    var bankId = Number(values[0]);
    var filterId = Number(values[1]);
    var parameterIndex = Number(values[2]);
    var normalized = Number(values[3]);
    var bank = this.LocalBank(bankId);
    var filter = bank && bank.filters[filterId];
    var definition = (this.filterDefinitions[filterId] || [])[parameterIndex];
    if (!bank || !bank.linkId || !filter || !definition ||
        !isFinite(normalized) || parameterIndex < 0 ||
        parameterIndex >= filter.values.length) return;

    var previousNormalized = NormalizeParameter(
        filter.values[parameterIndex], definition);
    var delta = normalized - previousNormalized;
    if (!isFinite(previousNormalized) || !isFinite(delta) || !delta) return;
    var update = {
        linkId: bank.linkId,
        bankId: bankId,
        filterId: filterId,
        parameterIndex: parameterIndex,
        delta: delta
    };
    filter.values[parameterIndex] = DenormalizeParameter(normalized, definition);
    this.ApplyFilterDeltaToModel(update, this.instanceId);
    outlet(1, "link.filter_delta", update.linkId, this.instanceId,
        this.NextLinkRevision(update.linkId), filterId, parameterIndex, delta);
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
        var normalized = NormalizeParameter(
            absolute, parameters[parameterIndex]);
        if (isFinite(normalized)) {
            this.HandleEqParameterGesture([
                bankId, filterId, parameterIndex, normalized
            ]);
        }
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
    outlet(1, "link.processor_delta", linkId, this.instanceId,
        this.NextLinkRevision(linkId), device, parameter, delta);
};

BankManager.prototype.HandleEqBypassGesture = function(values) {
    if (values.length !== 3) return;
    var bank = this.LocalBank(Number(values[0]));
    var filterId = Number(values[1]);
    var bypass = Number(values[2]) ? 1 : 0;
    if (!bank || !bank.linkId || !bank.filters[filterId]) return;
    this.PublishLinkBypass(bank.linkId, filterId, bypass);
};

BankManager.prototype.HandleEqFilterResetGesture = function(values) {
    if (values.length !== 2) return;
    var bank = this.LocalBank(Number(values[0]));
    var filterId = Number(values[1]);
    if (!bank || !bank.linkId || !bank.filters[filterId]) return;
    outlet(1, "link.filter_reset", bank.linkId, this.instanceId,
        this.NextLinkRevision(bank.linkId), filterId);
};

BankManager.prototype.HandleEqBankResetGesture = function(values) {
    if (values.length !== 1) return;
    var bank = this.LocalBank(Number(values[0]));
    if (!bank || !bank.linkId) return;
    outlet(1, "link.bank_reset", bank.linkId, this.instanceId,
        this.NextLinkRevision(bank.linkId));
};

BankManager.prototype.NextLinkRevision = function(linkId) {
    var next = (this.outgoingLinkRevisions[linkId] || 0) + 1;
    this.outgoingLinkRevisions[linkId] = next;
    return next;
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
    var stateParts = [this.instanceId, this.local.label, this.local.selectedBankId,
        this.local.systemBank.occupied ? 1 : 0];
    for (var stateIndex = 0; stateIndex < this.local.banks.length; stateIndex++) {
        stateParts.push(this.local.banks[stateIndex].occupied ? 1 : 0);
        stateParts.push(this.local.banks[stateIndex].linkId || "-");
    }
    var state = stateParts.join("|");
    if (state === this.lastAnnouncementState) return false;

    var fields = [this.instanceId, this.local.label, this.local.revision, this.local.selectedBankId,
        this.local.systemBank.occupied ? 1 : 0];
    for (var bankIndex = 0; bankIndex < this.local.banks.length; bankIndex++) {
        var bank = this.local.banks[bankIndex];
        fields.push(bank.id, bank.occupied ? 1 : 0, bank.linkId || "-");
    }
    this.lastAnnouncementState = state;
    outlet(1, "bank.announce", fields);
    this.PublishLinkedState();
    return true;
};

BankManager.prototype.PublishLinkedState = function() {
    for (var bankIndex = 0; bankIndex < this.local.banks.length; bankIndex++) {
        var bank = this.local.banks[bankIndex];
        if (!bank.linkId) continue;
        for (var filterId in bank.filters) {
            if (!bank.filters.hasOwnProperty(filterId)) continue;
            var filter = bank.filters[filterId];
            var fields = [
                bank.linkId, this.instanceId,
                this.local.revision, bank.id, Number(filterId),
                filter.bypass ? 1 : 0, filter.values.length
            ].concat(filter.values);
            outlet(1, "link.filter_state", fields);
        }
        for (var device in this.local.processors) {
            if (!this.local.processors.hasOwnProperty(device)) continue;
            var processor = this.local.processors[device];
            for (var parameter in processor.values) {
                if (!processor.values.hasOwnProperty(parameter) ||
                    !isFinite(processor.values[parameter])) continue;
                outlet(1, "link.processor_state", bank.linkId,
                    this.instanceId, this.local.revision, device,
                    parameter, processor.values[parameter]);
            }
        }
        outlet(1, "link.state_end", bank.linkId, this.instanceId,
            this.local.revision);
    }
};

BankManager.prototype.ParseAnnouncement = function(values) {
    if (values.length !== 23) return;
    var instanceId = String(values[0]);
    if (!instanceId || instanceId === this.instanceId) return;
    var revision = Number(values[2]);
    var selected = Number(values[3]);
    if (!isFinite(revision) || selected < 1 || selected > 6) return;
    var peer = this.peers[instanceId] || new InstanceSummary(instanceId, String(values[1]));
    if (revision < peer.revision) return;
    peer.label = String(values[1]);
    peer.revision = revision;
    peer.selectedBankId = selected;
    peer.systemBank.occupied = Number(values[4]) !== 0;
    var position = 5;
    for (var index = 0; index < 6; index++) {
        var bankId = Number(values[position++]);
        var occupied = Number(values[position++]) !== 0;
        var linkId = this.NormalizeLinkId(values[position++]);
        if (bankId !== index + 1) return;
        if (peer.banks[index].linkId !== linkId) {
            peer.banks[index].filters = {};
        }
        peer.banks[index].occupied = occupied;
        peer.banks[index].linkId = linkId;
    }
    this.peers[instanceId] = peer;
    this.RebuildProcessorLinkGroups();
};

BankManager.prototype.ApplyLinkFilterState = function(values) {
    if (values.length < 7) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var bankId = Number(values[3]);
    var filterId = Number(values[4]);
    var bypass = Number(values[5]) !== 0;
    var valueCount = Number(values[6]);
    var peer = this.peers[sourceId];
    if (!peer || sourceId === this.instanceId ||
        !isFinite(bankId) || !isFinite(filterId) ||
        !isFinite(valueCount) || valueCount < 0 ||
        values.length !== 7 + valueCount) return;
    var bank = peer.banks[bankId - 1];
    if (!bank || bank.linkId !== linkId) return;
    var filterValues = [];
    for (var index = 0; index < valueCount; index++) {
        var value = Number(values[7 + index]);
        if (!isFinite(value)) return;
        filterValues.push(value);
    }
    bank.filters[filterId] = {
        bypass: bypass,
        values: filterValues
    };
};

BankManager.prototype.ApplyLinkProcessorState = function(values) {
    if (values.length !== 6) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var device = String(values[3]);
    var parameter = String(values[4]);
    var value = Number(values[5]);
    var peer = this.peers[sourceId];
    if (!peer || sourceId === this.instanceId || !isFinite(value) ||
        !this.HasLink(peer, linkId) || !peer.processors[device]) return;
    peer.processors[device].values[parameter] = value;
};

BankManager.prototype.HandleGlobal = function(name, values) {
    var shouldRedraw = name !== "link.filter_delta" &&
        name !== "link.processor_delta" &&
        name !== "link.filter_state" &&
        name !== "link.processor_state" &&
        name !== "link.state_end";
    if (name === "bank.query") {
        if (String(values[0]) !== this.instanceId) {
            this.lastAnnouncementState = "";
            this.PublishAnnouncement();
        }
    } else if (name === "bank.announce") {
        this.ParseAnnouncement(values);
    } else if (name === "link.filter_state") {
        this.ApplyLinkFilterState(values);
    } else if (name === "link.processor_state") {
        this.ApplyLinkProcessorState(values);
    } else if (name === "link.state_end") {
        if (values.length === 3 &&
            String(values[1]) !== this.instanceId &&
            this.ActiveLinkId(this.local) === String(values[0])) {
            this.controlLinkSession = "";
            this.RefreshControlLinkSession();
        }
    } else if (name === "bank.leave") {
        this.RemovePeer(values);
    } else if (name === "link.create") {
        this.ApplyLinkCreate(values);
    } else if (name === "link.remove") {
        this.ApplyLinkRemoval(values);
    } else if (name === "link.join") {
        this.ApplyLinkJoin(values);
    } else if (name === "link.filter_delta") {
        this.ApplyFilterDelta(values);
    } else if (name === "link.filter_bypass") {
        this.ApplyLinkBypass(values);
    } else if (name === "link.filter_reset") {
        this.ApplyLinkFilterReset(values);
    } else if (name === "link.bank_reset") {
        this.ApplyLinkBankReset(values);
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
    this.SendProcessorValue(device, parameter, processor.values[parameter]);
};

BankManager.prototype.RemovePeer = function(values) {
    if (values.length !== 1) return;
    var instanceId = String(values[0]);
    if (!instanceId || instanceId === this.instanceId) return;
    delete this.peers[instanceId];
    this.RebuildProcessorLinkGroups();
    for (var key in this.linkSelection) {
        if (this.linkSelection[key].instanceId === instanceId) delete this.linkSelection[key];
    }
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

BankManager.prototype.ApplyLinkFilterReset = function(values) {
    if (values.length !== 4) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var filterId = Number(values[3]);
    if (sourceId === this.instanceId || !isFinite(revision) ||
        !isFinite(filterId)) return;
    var bank = this.FindLocalLinkedBank(linkId);
    if (bank && bank.filters[filterId] &&
        this.AcceptIncomingLinkRevision(
            linkId, sourceId, revision)) {
        this.SendHostCommand("eq.reset_filter", [bank.id, filterId]);
    }
};

BankManager.prototype.ApplyLinkBankReset = function(values) {
    if (values.length !== 3) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    if (sourceId === this.instanceId || !isFinite(revision)) return;
    var bank = this.FindLocalLinkedBank(linkId);
    if (bank && this.AcceptIncomingLinkRevision(
        linkId, sourceId, revision)) {
        this.SendHostCommand("eq.reset", [bank.id]);
    }
};

BankManager.prototype.ApplyLinkCreate = function(values) {
    if (values.length < 4) return;
    var linkId = String(values[0]);
    var count = Number(values[2]);
    if (!linkId || !isFinite(count) || count < 2 || values.length !== 3 + count * 2) return;
    for (var index = 0; index < count; index++) {
        var instanceId = String(values[3 + index * 2]);
        var entity = String(values[4 + index * 2]);
        if (instanceId !== this.instanceId) continue;
        if (entity.indexOf("bank:") === 0) {
            var bankId = Number(entity.substring(5));
            if (bankId >= 1 && bankId <= 6) this.SendHostCommand("eq.set_link", [bankId, linkId]);
        }
    }
};

BankManager.prototype.ApplyLinkRemoval = function(values) {
    if (values.length < 1) return;
    var linkId = String(values[0]);
    for (var index = 0; index < this.local.banks.length; index++) {
        if (this.local.banks[index].linkId === linkId) this.SendHostCommand("eq.set_link", [index + 1, "-"]);
    }
};

BankManager.prototype.ApplyLinkJoin = function(values) {
    if (values.length !== 3) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    if (!this.AcceptIncomingLinkRevision(
        linkId, sourceId, revision)) return;

    var bankIds = [];
    for (var index = 0; index < this.local.banks.length; index++) {
        var bank = this.local.banks[index];
        if (bank.linkId === linkId && bank.occupied) bankIds.push(bank.id);
    }
    if (bankIds.length === 0) return;

    this.SendHostCommand("eq.join_banks", [bankIds.length].concat(bankIds));
};

BankManager.prototype.SelectionKey = function(instanceId, entity) {
    return String(instanceId) + ":" + String(entity);
};

BankManager.prototype.ToggleLinkSelection = function(instanceId, bankId) {
    var entity = "bank:" + bankId;
    var key = this.SelectionKey(instanceId, entity);
    if (this.linkSelection[key]) delete this.linkSelection[key];
    else this.linkSelection[key] = { instanceId: instanceId, entity: entity };
};

BankManager.prototype.CanCreateLinkFromSelection = function() {
    var members = [];
    for (var key in this.linkSelection) members.push(this.linkSelection[key]);
    if (members.length < 2) return false;
    var instances = {};
    for (var index = 0; index < members.length; index++) {
        var member = members[index];
        var instance = member.instanceId === this.instanceId
            ? this.local
            : this.peers[member.instanceId];
        if (!instance || member.entity.indexOf("bank:") !== 0 || instances[member.instanceId]) return false;
        instances[member.instanceId] = true;
        var bankId = Number(member.entity.substring(5));
        var bank = instance.banks[bankId - 1];
        if (!bank || bank.linkId) return false;
    }
    return true;
};

BankManager.prototype.SelectedLocalBankIds = function() {
    var ids = [];
    for (var key in this.joinSelection) ids.push(Number(this.joinSelection[key]));
    if (ids.length === 0) ids.push(this.local.selectedBankId);
    return ids.sort(function(left, right) { return left - right; });
};

BankManager.prototype.ToggleJoinSelection = function(bankId) {
    var bank = this.LocalBank(bankId);
    if (!bank || !bank.occupied) return;
    if (this.joinSelection[bankId]) delete this.joinSelection[bankId];
    else this.joinSelection[bankId] = bankId;
};

BankManager.prototype.ActionStates = function() {
    var selected = this.SelectedLocalBankIds();
    var join = false;
    for (var index = 0; index < selected.length; index++) {
        var bank = this.LocalBank(selected[index]);
        if (!bank) {
            join = false;
            break;
        }
        if (bank.occupied) join = true;
    }

    var activeBank = this.LocalBank(this.local.selectedBankId);
    var unlink = Boolean(activeBank && activeBank.linkId);
    for (var selectionKey in this.linkSelection) {
        var selection = this.linkSelection[selectionKey];
        var instance = selection.instanceId === this.instanceId ? this.local : this.peers[selection.instanceId];
        var bankId = Number(selection.entity.substring(5));
        if (instance && instance.banks[bankId - 1] &&
            instance.banks[bankId - 1].linkId) unlink = true;
    }
    return {
        join: join,
        commit: Boolean(this.local.systemBank && this.local.systemBank.occupied && activeBank &&
            !activeBank.occupied && !activeBank.linkId),
        link: this.CanCreateLinkFromSelection(),
        unlink: unlink
    };
};

BankManager.prototype.HandleAction = function(action) {
    var states = this.ActionStates();
    if (!states[action]) return;
    var selected = this.SelectedLocalBankIds();
    if (action === "join") {
        var linked = {};
        var localBankIds = [];
        for (var index = 0; index < selected.length; index++) {
            var selectedBank = this.LocalBank(selected[index]);
            if (!selectedBank || !selectedBank.occupied) continue;
            if (selectedBank.linkId) linked[selectedBank.linkId] = true;
            else localBankIds.push(selectedBank.id);
        }
        for (var linkId in linked) {
            outlet(1, "link.join", linkId, this.instanceId, this.NextLinkRevision(linkId));
        }
        if (localBankIds.length > 0) {
            this.SendHostCommand("eq.join_banks", [localBankIds.length].concat(localBankIds));
        }
        this.joinSelection = {};
    } else if (action === "commit") {
        this.SendHostCommand("eq.commit_hidden", [this.local.selectedBankId]);
    } else if (action === "link") {
        outlet(1, "bank.query", this.instanceId);
        var members = [];
        for (var key in this.linkSelection) members.push(this.linkSelection[key]);
        if (members.length < 2) return;
        this.linkRevision += 1;
        var linkId = String(this.instanceId) + "." + String(Date.now()) + "." + String(this.linkRevision);
        var fields = [linkId, this.linkRevision % BankManagerColors.linkColors.length, members.length];
        for (var index = 0; index < members.length; index++) fields.push(members[index].instanceId, members[index].entity);
        outlet(1, "link.create", fields);
        this.linkSelection = {};
    } else if (action === "unlink") {
        var removalLinks = {};
        for (var selectionKey in this.linkSelection) {
            var selection = this.linkSelection[selectionKey];
            var selectedInstance = selection.instanceId === this.instanceId ? this.local : this.peers[selection.instanceId];
            if (!selectedInstance) continue;
            var selectedLink = (selectedInstance.banks[
                Number(selection.entity.substring(5)) - 1
            ] || {}).linkId;
            if (selectedLink) removalLinks[selectedLink] = true;
        }
        if (!Object.keys(removalLinks).length) {
            var bank = this.LocalBank(this.local.selectedBankId);
            if (bank && bank.linkId) removalLinks[bank.linkId] = true;
        }
        for (var removalLink in removalLinks) outlet(1, "link.remove", removalLink);
    }
};

BankManager.prototype.LocalBank = function(bankId) {
    return this.local.banks[bankId - 1] || null;
};

BankManager.prototype.Rows = function() {
    var rows = [this.local];
    var ids = Object.keys(this.peers).sort();
    for (var index = 0; index < ids.length; index++) {
        rows.push(this.peers[ids[index]]);
    }
    return rows;
};

BankManager.prototype.BankStartX = function(width) {
    var options = BankManagerVisualOptions;
    return options.padding;
};

BankManager.prototype.DrawSquare = function(instance, bank, x, y, local, interactive) {
    var options = BankManagerVisualOptions;
    var colors = BankManagerColors;
    var selected = interactive && this.linkSelection[this.SelectionKey(instance.id, "bank:" + bank.id)] !== undefined;
    var joinSelected = interactive && local && this.joinSelection[bank.id] !== undefined;
    var isActive = interactive && local && bank.id === this.local.selectedBankId;
    var activeLinkId = this.ActiveLinkId(this.local);
    var isActiveLinkMember = Boolean(activeLinkId && bank.linkId === activeLinkId);
    var systemOccupied = bank.id === 0 && bank.occupied;
    var isInactive = !interactive || !bank.occupied;
    var bankColor = bank.id === 0 ? colors.systemBank : colors.inactiveBank;
    if (interactive && bank.occupied) bankColor = colors.bankDefault;
    if (interactive && bank.linkId) {
        bankColor = colors.linkColors[Math.abs(this.Hash(bank.linkId)) % colors.linkColors.length];
    }
    if (selected) bankColor = colors.linkSelection;
    else if (joinSelected) bankColor = colors.joinSelection;
    var textColor = interactive ? bankColor : colors.disabledText;
    var isFilled = isActive || isActiveLinkMember || selected || joinSelected || systemOccupied;

    if (isFilled) {
        mgraphics.set_source_rgba(bankColor);
        mgraphics.rectangle(x, y, options.squareSize, options.squareSize);
        mgraphics.fill();
        textColor = colors.background;
    }
    var inactiveColor = bank.id === 0 ? colors.systemBank : colors.inactiveBank;
    mgraphics.set_source_rgba(isInactive && !selected && !joinSelected ? inactiveColor : bankColor);
    mgraphics.set_line_width(options.bankLineWidth);
    mgraphics.rectangle(x + 0.5, y + 0.5, options.squareSize - 1, options.squareSize - 1);
    mgraphics.stroke();

    mgraphics.set_source_rgba(textColor);
    mgraphics.set_font_size(options.bankFontSize);
    var label = String(bank.id);
    var width = mgraphics.text_measure(label)[0];
    mgraphics.move_to(x + (options.squareSize - width) * 0.5, y + options.squareSize * 0.67);
    mgraphics.show_text(label);
};

BankManager.prototype.Hash = function(value) {
    var hash = 0;
    for (var index = 0; index < value.length; index++) hash = ((hash << 5) - hash) + value.charCodeAt(index);
    return hash;
};

BankManager.prototype.ActionY = function() {
    var options = BankManagerVisualOptions;
    return Math.max(options.padding, mgraphics.size[1] - options.actionHeight - options.padding);
};

BankManager.prototype.ContentHeight = function() {
    return Math.max(0, this.ActionY() - BankManagerVisualOptions.padding);
};

BankManager.prototype.MaximumScrollOffset = function() {
    return Math.max(0, this.Rows().length * BankManagerVisualOptions.rowHeight - this.ContentHeight());
};

BankManager.prototype.Scroll = function(delta) {
    var step = Number(delta);
    if (!isFinite(step) || step === 0) return;
    this.scrollOffset = Math.max(0, Math.min(this.MaximumScrollOffset(),
        this.scrollOffset - step * BankManagerVisualOptions.rowHeight));
    mgraphics.redraw();
};

BankManager.prototype.Paint = function() {
    var width = mgraphics.size[0];
    var height = mgraphics.size[1];
    var options = BankManagerVisualOptions;
    var colors = BankManagerColors;
    mgraphics.set_source_rgba(colors.background);
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();
    mgraphics.select_font_face("Ableton Sans", "normal", "normal");
    var rows = this.Rows();
    var actionY = this.ActionY();
    this.scrollOffset = Math.min(this.scrollOffset, this.MaximumScrollOffset());
    var y = options.padding - this.scrollOffset;
    for (var rowIndex = 0; rowIndex < rows.length; rowIndex++) {
        if (y + options.rowHeight <= options.padding) {
            y += options.rowHeight;
            continue;
        }
        if (y >= actionY) break;
        var instance = rows[rowIndex];
        mgraphics.set_source_rgba(rowIndex === 0 ? colors.bankDefault : colors.instanceText);
        mgraphics.select_font_face("Ableton Sans", "normal",
            rowIndex === 0 ? options.currentLabelWeight : options.labelWeight);
        mgraphics.set_font_size(options.labelFontSize);
        mgraphics.move_to(options.padding, y + 13);
        var bankStartX = this.BankStartX(width);
        mgraphics.show_text(this.FitText(instance.label, width - options.padding * 2));
        mgraphics.select_font_face("Ableton Sans", "normal", options.bankFontWeight);
        var squareY = y + 23;
        var displayedBanks = [instance.systemBank].concat(instance.banks);
        for (var bankIndex = 0; bankIndex < displayedBanks.length; bankIndex++) {
            var x = bankStartX + bankIndex * (options.squareSize + options.squareGap);
            var interactive = displayedBanks[bankIndex].id !== 0;
            this.DrawSquare(instance, displayedBanks[bankIndex], x, squareY, rowIndex === 0, interactive);
        }
        mgraphics.set_source_rgba(rowIndex === 0 ? colors.currentSeparator : colors.separator);
        mgraphics.rectangle(options.padding, y + options.rowHeight - options.separatorWidth,
            width - options.padding * 2, options.separatorWidth);
        mgraphics.fill();
        y += options.rowHeight;
    }
    this.DrawActions(width, actionY);
};

BankManager.prototype.DrawActions = function(width, y) {
    var options = BankManagerVisualOptions;
    var colors = BankManagerColors;
    var labels = ["JOIN", "COMMIT", "LINK", "UNLINK"];
    var actions = ["join", "commit", "link", "unlink"];
    var states = this.ActionStates();
    var buttonWidth = (width - BankManagerVisualOptions.padding * 2 - 3 * options.actionGap) / labels.length;
    for (var index = 0; index < labels.length; index++) {
        var x = BankManagerVisualOptions.padding + index * (buttonWidth + options.actionGap);
        var enabled = states[actions[index]];
        mgraphics.set_source_rgba(enabled ? colors.actionFill : colors.disabledFill);
        mgraphics.rectangle(x, y, buttonWidth, BankManagerVisualOptions.actionHeight);
        mgraphics.fill();
        mgraphics.set_source_rgba(enabled ? colors.actionBorder : colors.disabledText);
        mgraphics.set_line_width(1);
        mgraphics.rectangle(x + .5, y + .5, buttonWidth - 1, BankManagerVisualOptions.actionHeight - 1);
        mgraphics.stroke();
        mgraphics.set_source_rgba(enabled ? colors.actionText : colors.disabledText);
        mgraphics.set_font_size(BankManagerVisualOptions.actionFontSize);
        var labelWidth = mgraphics.text_measure(labels[index])[0];
        mgraphics.move_to(x + (buttonWidth - labelWidth) * .5, y + 14);
        mgraphics.show_text(labels[index]);
    }
};

BankManager.prototype.FitText = function(value, maximumWidth) {
    var text = String(value);
    while (text.length > 1 && mgraphics.text_measure(text)[0] > maximumWidth) text = text.substring(0, text.length - 1);
    return text === value ? text : text.substring(0, Math.max(1, text.length - 3)) + "...";
};

BankManager.prototype.Click = function(x, y, ctrl, cmd, shift) {
    var options = BankManagerVisualOptions;
    var rows = this.Rows();
    var actionY = this.ActionY();
    var rowIndex = Math.floor((y - options.padding + this.scrollOffset) / options.rowHeight);
    if (y >= options.padding && y < actionY && rowIndex >= 0 && rowIndex < rows.length) {
        var bankOffset = x - this.BankStartX(mgraphics.size[0]);
        var bankIndex = Math.floor(bankOffset / (options.squareSize + options.squareGap));
        var displayedBanks = [rows[rowIndex].systemBank].concat(rows[rowIndex].banks);
        if (bankIndex >= 0 && bankIndex < displayedBanks.length) {
            var instance = rows[rowIndex];
            var bank = displayedBanks[bankIndex];
            if (bank.id === 0) return;
            if (ctrl || cmd) {
                if (!bank.linkId) {
                    this.ToggleLinkSelection(instance.id, bank.id);
                }
            } else if (shift && rowIndex === 0) {
                this.ToggleJoinSelection(bank.id);
            } else if (rowIndex === 0) {
                this.SendHostCommand("eq.select_bank", [bank.id]);
            }
            mgraphics.redraw();
            return;
        }
    }
    if (y < actionY || y > actionY + options.actionHeight) return;
    var buttonWidth = (mgraphics.size[0] - options.padding * 2 - 3 * options.actionGap) / 4;
    var actionIndex = Math.floor((x - options.padding) / (buttonWidth + options.actionGap));
    var actions = ["join", "commit", "link", "unlink"];
    if (actionIndex >= 0 && actionIndex < actions.length) this.HandleAction(actions[actionIndex]);
};

var bankManager = new BankManager();

function inletassist(index) {
    assist(index === 0
        ? "Local input: Host snapshots; eq_parameter_gesture, eq_parameter_absolute_gesture, eq_bypass_gesture, eq_filter_reset_gesture, eq_bank_reset_gesture, processor_parameter_gesture"
        : "Global bus: bank.*, link.create, link.remove, link.join, link.filter_*, link.bank_reset, link.processor_*, link.state_end");
}

function outletassist(index) {
    assist([
        "Host commands: eq.select_bank, eq.join_banks, eq.commit_hidden, eq.set_link, gain.set_parameter, compressor.set_parameter, saturator.set_parameter",
        "Global bus: bank.*, link.create, link.remove, link.join, link.filter_*, link.bank_reset, link.processor_*, link.state_end",
        "Local link UI: link_color, processor_limits, filter_limits"
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
        else if (String(values[3]) === "definitions") bankManager.ParseDefinitions(values);
        else if (String(values[3]) === "processor_definitions") bankManager.ParseProcessorDefinitions(values);
        else if (String(values[3]) === "processor") bankManager.ParseProcessorSnapshot(values);
        else if (String(values[3]) === "device") bankManager.ParseDeviceSnapshot(values);
        mgraphics.redraw();
    }
}
function event() {}
function eq_parameter_gesture() {
    if (inlet === 0) {
        bankManager.HandleEqParameterGesture(arrayfromargs(arguments));
    }
}
function eq_parameter_absolute_gesture() {
    if (inlet === 0) {
        bankManager.HandleEqAbsoluteParameterGesture(
            arrayfromargs(arguments));
    }
}
function eq_bypass_gesture() {
    if (inlet === 0) {
        bankManager.HandleEqBypassGesture(arrayfromargs(arguments));
    }
}
function eq_filter_reset_gesture() {
    if (inlet === 0) {
        bankManager.HandleEqFilterResetGesture(arrayfromargs(arguments));
    }
}
function eq_bank_reset_gesture() {
    if (inlet === 0) {
        bankManager.HandleEqBankResetGesture(arrayfromargs(arguments));
    }
}
function processor_parameter_gesture() {
    if (inlet === 0) {
        bankManager.HandleProcessorParameterGesture(arrayfromargs(arguments));
    }
}
function anything() {
    var values = arrayfromargs(arguments);
    if (inlet === 0 && messagename === "snapshot") {
        var snapshotValues = ["snapshot"].concat(values);
        if (String(snapshotValues[3]) === "eq") bankManager.ParseEqSnapshot(snapshotValues);
        else if (String(snapshotValues[3]) === "definitions") bankManager.ParseDefinitions(snapshotValues);
        else if (String(snapshotValues[3]) === "processor_definitions") bankManager.ParseProcessorDefinitions(snapshotValues);
        else if (String(snapshotValues[3]) === "processor") bankManager.ParseProcessorSnapshot(snapshotValues);
        else if (String(snapshotValues[3]) === "device") bankManager.ParseDeviceSnapshot(snapshotValues);
    } else if (inlet === 0 && messagename === "eq_parameter_gesture") {
        bankManager.HandleEqParameterGesture(values);
    } else if (inlet === 0 &&
        messagename === "eq_parameter_absolute_gesture") {
        bankManager.HandleEqAbsoluteParameterGesture(values);
    } else if (inlet === 0 && messagename === "eq_bypass_gesture") {
        bankManager.HandleEqBypassGesture(values);
    } else if (inlet === 0 && messagename === "eq_filter_reset_gesture") {
        bankManager.HandleEqFilterResetGesture(values);
    } else if (inlet === 0 && messagename === "eq_bank_reset_gesture") {
        bankManager.HandleEqBankResetGesture(values);
    } else if (inlet === 0 && messagename === "processor_parameter_gesture") {
        bankManager.HandleProcessorParameterGesture(values);
    } else if (inlet === 1) {
        bankManager.HandleGlobal(messagename, values);
    }
}
function list() {
    var values = arrayfromargs(arguments);
    if (inlet === 0 && values.length && String(values[0]) === "snapshot") {
        if (String(values[3]) === "eq") bankManager.ParseEqSnapshot(values);
        else if (String(values[3]) === "definitions") bankManager.ParseDefinitions(values);
        else if (String(values[3]) === "processor_definitions") bankManager.ParseProcessorDefinitions(values);
        else if (String(values[3]) === "processor") bankManager.ParseProcessorSnapshot(values);
        else if (String(values[3]) === "device") bankManager.ParseDeviceSnapshot(values);
    }
}
function leave() {
    if (bankManager.instanceId) outlet(1, "bank.leave", bankManager.instanceId);
}
