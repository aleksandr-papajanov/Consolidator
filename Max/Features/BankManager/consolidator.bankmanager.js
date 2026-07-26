autowatch = 1;
inlets = 2;
outlets = 3;

// Inlet 0: Host definitions, processor_definitions, EQ, DSP, and device snapshots.
// Inlet 1: global bank/link messages: bank.query, bank.announce, bank.leave,
// link.create, link.remove, link.join, link.parameter, link.filter_bypass,
// and link.processor_delta.
// Outlet 0: Host commands: eq.select_bank, eq.join_banks, eq.commit_hidden,
// eq.set_link, processor.set_link, gain.set_parameter, compressor.set_parameter,
// and saturator.set_parameter.
// Outlet 1: the complete global bank/link message set accepted by inlet 1.
// Outlet 2: processor_limits <device> <parameter> <absoluteMinimum> <absoluteMaximum>.
// bank.announce: <instanceId> <label> <revision> <selectedBank> <systemOccupied>
// <sixOccupancy> <sixBankLinks> <input|compressor|saturator|outputLinks>
// <inputGain> <compressor attack release input output mix>
// <saturator input output> <outputGain>.
// link.create: <linkId> <colorIndex> <count> <instanceId> <bank:N|processor:device>...
// link.parameter: <linkId> <sourceId> <revision> <bankId> <filterId> <parameterIndex> <value>.
// link.filter_bypass: <linkId> <sourceId> <revision> <bankId> <filterId> <0|1>.
// link.processor_delta: <linkId> <sourceId> <revision> <device> <parameter> <delta>.

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;
include("../Shared/JS/LinkColors.js");
include("JS/BankManagerTheme.js");
var BankManagerVisualOptions = BankManagerTheme.geometry;
var BankManagerColors = BankManagerTheme.colors;

function BankSummary() {
    this.id = 0;
    this.occupied = false;
    this.linkId = "";
    this.filters = {};
}

function ProcessorSummary(id, label) {
    this.id = id;
    this.label = label;
    this.linkId = "";
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

ProcessorLinkGroup.prototype.CanApply = function(sourceId, parameter, delta, range) {
    if (!range || Object.keys(this.members).length < 2) return false;
    for (var instanceId in this.members) {
        if (!this.members.hasOwnProperty(instanceId) || instanceId === sourceId) continue;
        var value = Number(this.members[instanceId].values[parameter]);
        if (!isFinite(value) || value + delta < range.minimum || value + delta > range.maximum) {
            return false;
        }
    }
    return true;
};

ProcessorLinkGroup.prototype.EffectiveRange = function(sourceId, parameter, range) {
    var source = this.members[sourceId];
    var sourceValue = source ? Number(source.values[parameter]) : NaN;
    if (!range || !isFinite(sourceValue) || Object.keys(this.members).length < 2) return range;
    var minimumDelta = -Infinity;
    var maximumDelta = Infinity;
    for (var instanceId in this.members) {
        if (!this.members.hasOwnProperty(instanceId) || instanceId === sourceId) continue;
        var value = Number(this.members[instanceId].values[parameter]);
        if (!isFinite(value)) return { minimum: sourceValue, maximum: sourceValue };
        minimumDelta = Math.max(minimumDelta, range.minimum - value);
        maximumDelta = Math.min(maximumDelta, range.maximum - value);
    }
    return {
        minimum: Math.max(range.minimum, sourceValue + minimumDelta),
        maximum: Math.min(range.maximum, sourceValue + maximumDelta)
    };
};

ProcessorLinkGroup.prototype.ApplyDelta = function(sourceId, parameter, delta, sourceAlreadyApplied) {
    for (var instanceId in this.members) {
        if (!this.members.hasOwnProperty(instanceId) ||
            (sourceAlreadyApplied && instanceId === sourceId)) continue;
        var processor = this.members[instanceId];
        if (isFinite(processor.values[parameter])) processor.values[parameter] += delta;
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
    this.linkRevisions = {};
    this.filterParameters = {};
    this.processorRanges = {};
    this.processorLinkGroups = {};
    this.processorLimitSessions = {};
    this.remoteChanges = {};
    this.remoteProcessorChanges = {};
    this.pendingLinkJoins = {};
    this.pendingLinkParameters = {};
    this.linkParameterFlushScheduled = false;
    this.linkParameterFlushTask = new Task(this.FlushLinkParameters, this);
    this.pendingProcessorDeltas = {};
    this.processorDeltaFlushScheduled = false;
    this.processorDeltaFlushTask = new Task(this.FlushProcessorDeltas, this);
    this.scrollOffset = 0;
    this.lastAnnouncementState = "";
}

BankManager.prototype.Initialize = function() {
    this.instanceId = this.CurrentRuntimeInstanceId();
    this.local.id = this.instanceId;
    this.local.label = this.CurrentLabel();
    if (!this.instanceId) return;
    outlet(1, "bank.query", this.instanceId);
    this.PublishAnnouncement();
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
    var previousBanks = this.local.banks;
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
            var parameterNames = this.filterParameters[filterId] || [];
            for (var valueIndex = 0; valueIndex < valueCount; valueIndex++) {
                var filterValue = Number(values[position + valueIndex]);
                filterValues.push(filterValue);
                if (parameterNames[valueIndex] === "gain" && Math.abs(filterValue) > 1.0e-12) occupied = true;
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
    this.PublishLinkedChanges(previousBanks, banks, this.pendingLinkJoins);
    this.pendingLinkJoins = {};
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
            parameters.push(String(values[position++]));
            position += 4;
        }
        this.filterParameters[filterId] = parameters;
    }
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
            position += 2;
            this.processorRanges[device][name] = { minimum: minimum, maximum: maximum };
        }
    }
    this.RefreshProcessorLimitSessions();
};

BankManager.prototype.ParseDspSnapshot = function(values) {
    if (values.length < 35 || String(values[3]) !== "dsp") return;
    var count = values.length;
    var base = count - 35;
    var previous = this.local.processors;
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
        input: Number(values[base + 4]),
        output: Number(values[base + 5]),
        mix: Number(values[base + 6])
    };
    processors.saturator.values = {
        input: Number(values[base + 18]),
        output: Number(values[base + 19])
    };
    processors.output_gain.values.gain = Number(values[base + 30]);
    processors.input_gain.linkId = this.NormalizeLinkId(values[count - 4]);
    processors.compressor.linkId = this.NormalizeLinkId(values[count - 3]);
    processors.saturator.linkId = this.NormalizeLinkId(values[count - 2]);
    processors.output_gain.linkId = this.NormalizeLinkId(values[count - 1]);
    var shouldAnnounce = !isFinite(previous.input_gain.values.gain);
    for (var device in processors) {
        if (processors.hasOwnProperty(device) &&
            previous[device].linkId !== processors[device].linkId) {
            shouldAnnounce = true;
        }
    }
    this.local.processors = processors;
    this.PublishProcessorChanges(previous, processors);
    this.RebuildProcessorLinkGroups();
    if (shouldAnnounce) this.PublishAnnouncement();
};

BankManager.prototype.ProcessorParameterKey = function(device, parameter) {
    return device + ":" + parameter;
};

BankManager.prototype.PublishProcessorChanges = function(previous, current) {
    for (var device in current) {
        if (!current.hasOwnProperty(device)) continue;
        var next = current[device];
        var prior = previous[device];
        if (!next.linkId || !prior || prior.linkId !== next.linkId) continue;
        for (var parameter in next.values) {
            if (!next.values.hasOwnProperty(parameter)) continue;
            var nextValue = next.values[parameter];
            var priorValue = prior.values[parameter];
            if (nextValue === priorValue) continue;
            var key = this.ProcessorParameterKey(device, parameter);
            if (this.remoteProcessorChanges[key] === nextValue) {
                delete this.remoteProcessorChanges[key];
                continue;
            }
            this.QueueProcessorDelta(
                next.linkId,
                device,
                parameter,
                nextValue - priorValue,
                priorValue
            );
        }
    }
};

BankManager.prototype.QueueProcessorDelta = function(linkId, device, parameter, delta, previousValue) {
    var key = [linkId, device, parameter].join(":");
    var pending = this.pendingProcessorDeltas[key];
    if (pending) {
        pending.delta += delta;
    } else {
        this.pendingProcessorDeltas[key] = {
            linkId: linkId,
            device: device,
            parameter: parameter,
            delta: delta,
            previousValue: previousValue
        };
    }
    if (this.processorDeltaFlushScheduled) return;
    this.processorDeltaFlushScheduled = true;
    this.processorDeltaFlushTask.schedule(16);
};

BankManager.prototype.FlushProcessorDeltas = function() {
    this.processorDeltaFlushScheduled = false;
    var pending = this.pendingProcessorDeltas;
    this.pendingProcessorDeltas = {};
    for (var key in pending) {
        if (!pending.hasOwnProperty(key)) continue;
        var update = pending[key];
        var processor = this.local.processors[update.device];
        if (!processor || processor.linkId !== update.linkId || update.delta === 0) continue;
        var group = this.ProcessorLinkGroup(update.linkId, update.device);
        var range = this.processorRanges[update.device] &&
            this.processorRanges[update.device][update.parameter];
        if (!group || !group.CanApply(
            this.instanceId,
            update.parameter,
            update.delta,
            range
        )) {
            this.remoteProcessorChanges[
                this.ProcessorParameterKey(update.device, update.parameter)
            ] = update.previousValue;
            this.SendProcessorValue(update.device, update.parameter, update.previousValue);
            continue;
        }
        group.ApplyDelta(
            this.instanceId,
            update.parameter,
            update.delta,
            true);
        outlet(1, "link.processor_delta", update.linkId, this.instanceId,
            this.NextLinkRevision(update.linkId), update.device, update.parameter, update.delta);
    }
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
        for (var device in instance.processors) {
            if (!instance.processors.hasOwnProperty(device)) continue;
            var processor = instance.processors[device];
            if (!processor.linkId) continue;
            var key = processor.linkId + ":" + device;
            if (!groups[key]) groups[key] = new ProcessorLinkGroup(processor.linkId, device);
            groups[key].AddMember(instance.id, processor);
        }
    }
    this.processorLinkGroups = groups;
    this.RefreshProcessorLimitSessions();
};

BankManager.prototype.ProcessorLinkGroup = function(linkId, device) {
    return this.processorLinkGroups[linkId + ":" + device] || null;
};

BankManager.prototype.RefreshProcessorLimitSessions = function() {
    for (var device in this.local.processors) {
        if (!this.local.processors.hasOwnProperty(device)) continue;
        var processor = this.local.processors[device];
        var definitions = this.processorRanges[device] || {};
        var group = processor.linkId ? this.ProcessorLinkGroup(processor.linkId, device) : null;
        var memberIds = group ? Object.keys(group.members).sort() : [];
        var isLinked = processor.linkId && memberIds.length >= 2;
        var signature = isLinked
            ? processor.linkId + ":" + memberIds.join(",")
            : "unlinked";
        if (this.processorLimitSessions[device] === signature) continue;
        for (var parameter in processor.values) {
            if (!processor.values.hasOwnProperty(parameter) || !definitions[parameter]) continue;
            var range = definitions[parameter];
            var effective = isLinked
                ? group.EffectiveRange(this.instanceId, parameter, range)
                : range;
            outlet(2, "processor_limits", device, parameter, effective.minimum, effective.maximum);
        }
        if (Object.keys(definitions).length > 0) {
            this.processorLimitSessions[device] = signature;
        }
    }
};

BankManager.prototype.ParseDeviceSnapshot = function(values) {
    if (values.length !== 6 || String(values[0]) !== "snapshot" || String(values[3]) !== "device") return;
    if (!this.instanceId) return;
    this.PublishAnnouncement();
};

BankManager.prototype.RemoteChangeKey = function(bankId, filterId, parameterIndex) {
    return [bankId, filterId, parameterIndex].join(":");
};

BankManager.prototype.PublishLinkedChanges = function(previousBanks, banks, suppressedLinkIds) {
    for (var bankIndex = 0; bankIndex < banks.length; bankIndex++) {
        var current = banks[bankIndex];
        var previous = previousBanks[bankIndex];
        if (!current.linkId || !previous || previous.linkId !== current.linkId || suppressedLinkIds[current.linkId]) continue;
        for (var filterId in current.filters) {
            var currentFilter = current.filters[filterId];
            var previousFilter = previous.filters[filterId];
            if (!previousFilter) continue;
            for (var valueIndex = 0; valueIndex < currentFilter.values.length; valueIndex++) {
                var value = currentFilter.values[valueIndex];
                if (previousFilter.values[valueIndex] === value) continue;
                var key = this.RemoteChangeKey(current.id, filterId, valueIndex);
                if (this.remoteChanges.hasOwnProperty(key) && this.remoteChanges[key] === value) {
                    delete this.remoteChanges[key];
                    continue;
                }
                delete this.remoteChanges[key];
                this.QueueLinkParameter(current.linkId, current.id, Number(filterId), valueIndex, value);
            }
            if (previousFilter.bypass !== currentFilter.bypass) {
                var bypassValue = currentFilter.bypass ? 1 : 0;
                var bypassKey = this.RemoteChangeKey(current.id, filterId, "bypass");
                if (this.remoteChanges.hasOwnProperty(bypassKey) && this.remoteChanges[bypassKey] === bypassValue) {
                    delete this.remoteChanges[bypassKey];
                } else {
                    delete this.remoteChanges[bypassKey];
                    this.PublishLinkBypass(current.linkId, current.id, Number(filterId), bypassValue);
                }
            }
        }
    }
};

BankManager.prototype.NextLinkRevision = function(linkId) {
    var next = Math.max(Date.now(), (this.linkRevisions[linkId] || 0) + 1);
    this.linkRevisions[linkId] = next;
    return next;
};

BankManager.prototype.PublishLinkParameter = function(linkId, bankId, filterId, parameterIndex, value) {
    outlet(1, "link.parameter", linkId, this.instanceId, this.NextLinkRevision(linkId),
        bankId, filterId, parameterIndex, value);
};

BankManager.prototype.QueueLinkParameter = function(linkId, bankId, filterId, parameterIndex, value) {
    var key = [linkId, bankId, filterId, parameterIndex].join(":");
    this.pendingLinkParameters[key] = {
        linkId: linkId,
        bankId: bankId,
        filterId: filterId,
        parameterIndex: parameterIndex,
        value: value
    };
    if (this.linkParameterFlushScheduled) return;
    this.linkParameterFlushScheduled = true;
    this.linkParameterFlushTask.schedule(16);
};

BankManager.prototype.FlushLinkParameters = function() {
    this.linkParameterFlushScheduled = false;
    var pending = this.pendingLinkParameters;
    this.pendingLinkParameters = {};
    for (var key in pending) {
        if (!pending.hasOwnProperty(key)) continue;
        var update = pending[key];
        this.PublishLinkParameter(
            update.linkId,
            update.bankId,
            update.filterId,
            update.parameterIndex,
            update.value
        );
    }
};

BankManager.prototype.PublishLinkBypass = function(linkId, bankId, filterId, bypass) {
    outlet(1, "link.filter_bypass", linkId, this.instanceId, this.NextLinkRevision(linkId),
        bankId, filterId, bypass);
};

BankManager.prototype.PublishAnnouncement = function() {
    if (!this.instanceId || this.local.banks.length !== 6) return;
    var fields = [this.instanceId, this.local.label, this.local.revision, this.local.selectedBankId,
        this.local.systemBank.occupied ? 1 : 0];
    for (var index = 0; index < this.local.banks.length; index++) fields.push(this.local.banks[index].occupied ? 1 : 0);
    for (var linkIndex = 0; linkIndex < this.local.banks.length; linkIndex++) {
        fields.push(this.local.banks[linkIndex].linkId || "-");
    }
    var processorIds = ["input_gain", "compressor", "saturator", "output_gain"];
    for (var processorIndex = 0; processorIndex < processorIds.length; processorIndex++) {
        fields.push(this.local.processors[processorIds[processorIndex]].linkId || "-");
    }
    fields.push(this.local.processors.input_gain.values.gain);
    fields.push(this.local.processors.compressor.values.attack);
    fields.push(this.local.processors.compressor.values.release);
    fields.push(this.local.processors.compressor.values.input);
    fields.push(this.local.processors.compressor.values.output);
    fields.push(this.local.processors.compressor.values.mix);
    fields.push(this.local.processors.saturator.values.input);
    fields.push(this.local.processors.saturator.values.output);
    fields.push(this.local.processors.output_gain.values.gain);
    for (var fieldIndex = 17; fieldIndex < fields.length; fieldIndex++) {
        if (fields[fieldIndex] === undefined) return;
    }
    var state = [fields[0], fields[1], fields[3]].concat(fields.slice(4)).join("|");
    if (state === this.lastAnnouncementState) return;
    this.lastAnnouncementState = state;
    outlet(1, "bank.announce", fields);
};

BankManager.prototype.ParseAnnouncement = function(values) {
    if (values.length !== 30) return;
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
    for (var index = 0; index < 6; index++) {
        peer.banks[index].occupied = Number(values[5 + index]) !== 0;
        peer.banks[index].linkId = String(values[11 + index]) === "-" ? "" : String(values[11 + index]);
    }
    peer.processors.input_gain.linkId = this.NormalizeLinkId(values[17]);
    peer.processors.compressor.linkId = this.NormalizeLinkId(values[18]);
    peer.processors.saturator.linkId = this.NormalizeLinkId(values[19]);
    peer.processors.output_gain.linkId = this.NormalizeLinkId(values[20]);
    peer.processors.input_gain.values.gain = Number(values[21]);
    peer.processors.compressor.values = {
        attack: Number(values[22]), release: Number(values[23]),
        input: Number(values[24]), output: Number(values[25]), mix: Number(values[26])
    };
    peer.processors.saturator.values = { input: Number(values[27]), output: Number(values[28]) };
    peer.processors.output_gain.values.gain = Number(values[29]);
    this.peers[instanceId] = peer;
    this.RebuildProcessorLinkGroups();
};

BankManager.prototype.HandleGlobal = function(name, values) {
    var shouldRedraw = name !== "link.parameter" &&
        name !== "link.processor_delta";
    if (name === "bank.query") {
        if (String(values[0]) !== this.instanceId) {
            this.lastAnnouncementState = "";
            this.PublishAnnouncement();
        }
    } else if (name === "bank.announce") {
        this.ParseAnnouncement(values);
    } else if (name === "bank.leave") {
        this.RemovePeer(values);
    } else if (name === "link.create") {
        this.ApplyLinkCreate(values);
    } else if (name === "link.remove") {
        this.ApplyLinkRemoval(values);
    } else if (name === "link.join") {
        this.ApplyLinkJoin(values);
    } else if (name === "link.parameter") {
        this.ApplyLinkParameter(values);
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
        revision <= (this.linkRevisions[linkId] || 0) || !isFinite(delta)) return;
    var group = this.ProcessorLinkGroup(linkId, device);
    if (!group) return;
    group.ApplyDelta(sourceId, parameter, delta, false);
    this.linkRevisions[linkId] = revision;
    var processor = this.local.processors[device];
    if (!processor || processor.linkId !== linkId) return;
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

BankManager.prototype.ApplyLinkParameter = function(values) {
    if (values.length !== 7) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var filterId = Number(values[4]);
    var parameterIndex = Number(values[5]);
    var value = Number(values[6]);
    if (sourceId === this.instanceId || !isFinite(revision) || revision <= (this.linkRevisions[linkId] || 0) ||
        !isFinite(filterId) || !isFinite(parameterIndex) || !isFinite(value)) return;
    this.linkRevisions[linkId] = revision;
    for (var index = 0; index < this.local.banks.length; index++) {
        var bank = this.local.banks[index];
        if (bank.linkId !== linkId) continue;
        this.remoteChanges[this.RemoteChangeKey(bank.id, filterId, parameterIndex)] = value;
        this.SendHostCommand("eq.set_parameter_index", [bank.id, filterId, parameterIndex, value]);
    }
};

BankManager.prototype.ApplyLinkBypass = function(values) {
    if (values.length !== 6) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var filterId = Number(values[4]);
    var bypass = Number(values[5]) ? 1 : 0;
    if (sourceId === this.instanceId || !isFinite(revision) || revision <= (this.linkRevisions[linkId] || 0) || !isFinite(filterId)) return;
    this.linkRevisions[linkId] = revision;
    for (var index = 0; index < this.local.banks.length; index++) {
        var bank = this.local.banks[index];
        if (bank.linkId !== linkId) continue;
        this.remoteChanges[this.RemoteChangeKey(bank.id, filterId, "bypass")] = bypass;
        this.SendHostCommand("eq.set_bypass", [bank.id, filterId, bypass]);
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
        } else if (entity.indexOf("processor:") === 0) {
            this.SendHostCommand("processor.set_link", [entity.substring(10), linkId]);
        }
    }
};

BankManager.prototype.ApplyLinkRemoval = function(values) {
    if (values.length < 1) return;
    var linkId = String(values[0]);
    for (var index = 0; index < this.local.banks.length; index++) {
        if (this.local.banks[index].linkId === linkId) this.SendHostCommand("eq.set_link", [index + 1, "-"]);
    }
    for (var device in this.local.processors) {
        if (this.local.processors[device].linkId === linkId) {
            this.SendHostCommand("processor.set_link", [device, "-"]);
        }
    }
};

BankManager.prototype.ApplyLinkJoin = function(values) {
    if (values.length !== 3) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var knownRevision = this.linkRevisions[linkId] || 0;
    if (!linkId || !isFinite(revision) || revision < knownRevision ||
        (revision === knownRevision && sourceId !== this.instanceId)) return;
    this.linkRevisions[linkId] = revision;

    var bankIds = [];
    for (var index = 0; index < this.local.banks.length; index++) {
        var bank = this.local.banks[index];
        if (bank.linkId === linkId && bank.occupied) bankIds.push(bank.id);
    }
    if (bankIds.length === 0) return;

    // Joining clears source banks locally. Those defaults are not link edits.
    this.pendingLinkJoins[linkId] = true;
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

BankManager.prototype.ToggleProcessorLinkSelection = function(instanceId, device) {
    var entity = "processor:" + device;
    var key = this.SelectionKey(instanceId, entity);
    if (this.linkSelection[key]) delete this.linkSelection[key];
    else this.linkSelection[key] = { instanceId: instanceId, entity: entity };
};

BankManager.prototype.CanCreateLinkFromSelection = function() {
    var members = [];
    for (var key in this.linkSelection) members.push(this.linkSelection[key]);
    if (members.length < 2) return false;
    var processorEntity = "";
    for (var index = 0; index < members.length; index++) {
        var member = members[index];
        var instance = member.instanceId === this.instanceId
            ? this.local
            : this.peers[member.instanceId];
        if (!instance) return false;
        if (member.entity.indexOf("processor:") === 0) {
            if (!processorEntity) processorEntity = member.entity;
            if (processorEntity !== member.entity) return false;
            var device = member.entity.substring(10);
            if (!instance.processors[device] || instance.processors[device].linkId) return false;
        } else {
            if (processorEntity) return false;
            var bankId = Number(member.entity.substring(5));
            var bank = instance.banks[bankId - 1];
            if (!bank || bank.occupied || bank.linkId) return false;
        }
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
        var parts = selection.entity.split(":");
        var instance = selection.instanceId === this.instanceId ? this.local : this.peers[selection.instanceId];
        if (!instance) continue;
        if (parts[0] === "processor" && instance.processors[parts[1]] &&
            instance.processors[parts[1]].linkId) unlink = true;
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
        this.RebuildProcessorLinkGroups();
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
            var parts = selection.entity.split(":");
            var selectedInstance = selection.instanceId === this.instanceId ? this.local : this.peers[selection.instanceId];
            if (!selectedInstance) continue;
            var selectedLink = parts[0] === "bank"
                ? (selectedInstance.banks[Number(parts[1]) - 1] || {}).linkId
                : (selectedInstance.processors[parts[1]] || {}).linkId;
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
    var isFilled = isActive || selected || joinSelected || systemOccupied;

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

BankManager.prototype.DrawProcessor = function(instance, processor, x, y) {
    var options = BankManagerVisualOptions;
    var colors = BankManagerColors;
    var selected = this.linkSelection[
        this.SelectionKey(instance.id, "processor:" + processor.id)] !== undefined;
    var color = processor.linkId
        ? colors.linkColors[Math.abs(this.Hash(processor.linkId)) % colors.linkColors.length]
        : colors.bankDefault;
    if (selected) color = colors.linkSelection;
    if (selected) {
        mgraphics.set_source_rgba(color);
        mgraphics.rectangle(x, y, options.processorWidth, options.squareSize);
        mgraphics.fill();
    }
    mgraphics.set_source_rgba(color);
    mgraphics.set_line_width(options.bankLineWidth);
    mgraphics.rectangle(x + 0.5, y + 0.5, options.processorWidth - 1, options.squareSize - 1);
    mgraphics.stroke();
    mgraphics.set_source_rgba(selected ? colors.background : color);
    mgraphics.set_font_size(options.processorFontSize);
    var labelWidth = mgraphics.text_measure(processor.label)[0];
    mgraphics.move_to(x + (options.processorWidth - labelWidth) * 0.5, y + options.squareSize * 0.67);
    mgraphics.show_text(processor.label);
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
        var processorX = bankStartX + displayedBanks.length * (options.squareSize + options.squareGap) + options.processorGap;
        var processorIds = ["compressor", "saturator", "input_gain", "output_gain"];
        for (var processorIndex = 0; processorIndex < processorIds.length; processorIndex++) {
            this.DrawProcessor(instance, instance.processors[processorIds[processorIndex]],
                processorX, squareY);
            processorX += options.processorWidth + options.processorGap;
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
                if (!bank.occupied && !bank.linkId) {
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
        var processorX = this.BankStartX(mgraphics.size[0])
            + displayedBanks.length * (options.squareSize + options.squareGap)
            + options.processorGap;
        var processorIds = ["compressor", "saturator", "input_gain", "output_gain"];
        for (var processorIndex = 0; processorIndex < processorIds.length; processorIndex++) {
            if (x >= processorX && x <= processorX + options.processorWidth) {
                if (ctrl || cmd) {
                    this.ToggleProcessorLinkSelection(rows[rowIndex].id, processorIds[processorIndex]);
                    mgraphics.redraw();
                }
                return;
            }
            processorX += options.processorWidth + options.processorGap;
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
        ? "Host snapshots: definitions, processor_definitions, eq, dsp, device"
        : "Global bus: bank.query, bank.announce, bank.leave, link.create, link.remove, link.join, link.parameter, link.filter_bypass, link.processor_delta");
}

function outletassist(index) {
    assist([
        "Host commands: eq.select_bank, eq.join_banks, eq.commit_hidden, eq.set_link, processor.set_link, gain.set_parameter, compressor.set_parameter, saturator.set_parameter",
        "Global bus: bank.query, bank.announce, bank.leave, link.create, link.remove, link.join, link.parameter, link.filter_bypass, link.processor_delta",
        "Local control limits: processor_limits <device> <parameter> <absoluteMinimum> <absoluteMaximum>"
    ][index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function loadbang() {}
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
        else if (String(values[3]) === "dsp") bankManager.ParseDspSnapshot(values);
        else if (String(values[3]) === "device") bankManager.ParseDeviceSnapshot(values);
        mgraphics.redraw();
    }
}
function anything() {
    var values = arrayfromargs(arguments);
    if (inlet === 0 && messagename === "snapshot") {
        var snapshotValues = ["snapshot"].concat(values);
        if (String(snapshotValues[3]) === "eq") bankManager.ParseEqSnapshot(snapshotValues);
        else if (String(snapshotValues[3]) === "definitions") bankManager.ParseDefinitions(snapshotValues);
        else if (String(snapshotValues[3]) === "processor_definitions") bankManager.ParseProcessorDefinitions(snapshotValues);
        else if (String(snapshotValues[3]) === "dsp") bankManager.ParseDspSnapshot(snapshotValues);
        else if (String(snapshotValues[3]) === "device") bankManager.ParseDeviceSnapshot(snapshotValues);
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
        else if (String(values[3]) === "dsp") bankManager.ParseDspSnapshot(values);
        else if (String(values[3]) === "device") bankManager.ParseDeviceSnapshot(values);
    }
}
function leave() {
    if (bankManager.instanceId) outlet(1, "bank.leave", bankManager.instanceId);
}
