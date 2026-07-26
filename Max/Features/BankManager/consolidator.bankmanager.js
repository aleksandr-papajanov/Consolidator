autowatch = 1;
inlets = 2;
outlets = 2;

// Inlet 0: local Host snapshots: snapshot 1 host eq <revision> <selected> <bypass> <solo> 7 <banks...>.
// Inlet 1: global messages: bank.query <instanceId>; bank.announce <instanceId> <label> <revision> <selected> <occupancy...> <links...>; bank.leave <instanceId>; link.create ...; link.remove ...; link.join <linkId> <sourceId> <revision>.
// Outlet 0: Host commands: command 1 bankmanager.ui <requestId> eq.select_bank|eq.join_banks|eq.commit_hidden|eq.set_link ... .
// Outlet 1: global bank messages: bank.query, bank.announce, bank.leave, link.create, link.remove, link.join.

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

var BankManagerVisualOptions = {
    background: [0.075, 0.075, 0.075, 1],
    row: [0.13, 0.13, 0.13, 1],
    active: [0.10, 0.45, 0.57, 1],
    border: [0.28, 0.28, 0.28, 1],
    empty: [0.12, 0.12, 0.12, 1],
    filled: [0.31, 0.31, 0.31, 1],
    text: [0.88, 0.88, 0.88, 1],
    muted: [0.58, 0.58, 0.58, 1],
    disabledText: [0.34, 0.34, 0.34, 1],
    disabledFill: [0.09, 0.09, 0.09, 1],
    hiddenFilled: [0.25, 0.25, 0.25, 1],
    selection: [0.95, 0.95, 0.95, 1],
    joinSelection: [0.98, 0.72, 0.16, 1],
    linkColors: [
        [0.10, 0.78, 0.92, 1], [0.93, 0.32, 0.50, 1], [0.98, 0.75, 0.12, 1],
        [0.45, 0.80, 0.30, 1], [0.66, 0.43, 0.90, 1], [0.96, 0.50, 0.16, 1]
    ],
    padding: 7,
    rowHeight: 33,
    squareSize: 20,
    squareGap: 5,
    actionHeight: 22
};

function BankSummary() {
    this.id = 0;
    this.occupied = false;
    this.linkId = "";
    this.filters = {};
}

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
    this.remoteChanges = {};
    this.pendingLinkJoins = {};
    this.scrollOffset = 0;
}

BankManager.prototype.Initialize = function() {
    this.local.label = this.CurrentLabel();
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

BankManager.prototype.ParseDeviceSnapshot = function(values) {
    if (values.length !== 6 || String(values[0]) !== "snapshot" || String(values[3]) !== "device") return;
    var instanceId = String(values[5]);
    if (!instanceId || instanceId === this.instanceId) return;
    this.instanceId = instanceId;
    this.local.id = instanceId;
    this.PublishAnnouncement();
    outlet(1, "bank.query", instanceId);
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
                this.PublishLinkParameter(current.linkId, current.id, Number(filterId), valueIndex, value);
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
    outlet(1, "bank.announce", fields);
};

BankManager.prototype.ParseAnnouncement = function(values) {
    if (values.length !== 17) return;
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
    this.peers[instanceId] = peer;
};

BankManager.prototype.HandleGlobal = function(name, values) {
    if (name === "bank.query") {
        if (String(values[0]) !== this.instanceId) this.PublishAnnouncement();
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
    }
    mgraphics.redraw();
};

BankManager.prototype.RemovePeer = function(values) {
    if (values.length !== 1) return;
    var instanceId = String(values[0]);
    if (!instanceId || instanceId === this.instanceId) return;
    delete this.peers[instanceId];
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
        var bankId = Number(values[4 + index * 2]);
        if (instanceId === this.instanceId && bankId >= 1 && bankId <= 6) {
            this.SendHostCommand("eq.set_link", [bankId, linkId]);
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

BankManager.prototype.SelectionKey = function(instanceId, bankId) {
    return String(instanceId) + ":" + String(bankId);
};

BankManager.prototype.ToggleLinkSelection = function(instanceId, bankId) {
    var key = this.SelectionKey(instanceId, bankId);
    if (this.linkSelection[key]) delete this.linkSelection[key];
    else this.linkSelection[key] = { instanceId: instanceId, bankId: bankId };
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
    return {
        join: join,
        commit: Boolean(this.local.systemBank && this.local.systemBank.occupied && activeBank &&
            !activeBank.occupied && !activeBank.linkId),
        link: Object.keys(this.linkSelection).length >= 2,
        unlink: Boolean(activeBank && activeBank.linkId)
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
        var members = [];
        for (var key in this.linkSelection) members.push(this.linkSelection[key]);
        if (members.length < 2) return;
        this.linkRevision += 1;
        var linkId = String(this.instanceId) + "." + String(Date.now()) + "." + String(this.linkRevision);
        var fields = [linkId, this.linkRevision % BankManagerVisualOptions.linkColors.length, members.length];
        for (var index = 0; index < members.length; index++) fields.push(members[index].instanceId, members[index].bankId);
        outlet(1, "link.create", fields);
        this.linkSelection = {};
    } else if (action === "unlink") {
        var bank = this.LocalBank(this.local.selectedBankId);
        if (bank && bank.linkId) outlet(1, "link.remove", bank.linkId);
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

BankManager.prototype.DrawSquare = function(instance, bank, x, y, local, interactive) {
    var options = BankManagerVisualOptions;
    var selected = interactive && this.linkSelection[this.SelectionKey(instance.id, bank.id)] !== undefined;
    var joinSelected = interactive && local && this.joinSelection[bank.id] !== undefined;
    mgraphics.set_source_rgba(interactive
        ? (bank.occupied ? options.filled : options.empty)
        : (bank.occupied ? options.hiddenFilled : options.disabledFill));
    mgraphics.rectangle(x, y, options.squareSize, options.squareSize);
    mgraphics.fill();
    if (interactive && bank.linkId) {
        var colorIndex = Math.abs(this.Hash(bank.linkId)) % options.linkColors.length;
        mgraphics.set_source_rgba(options.linkColors[colorIndex]);
        mgraphics.rectangle(x, y, 3, options.squareSize);
        mgraphics.fill();
    }
    if (interactive && local && bank.id === this.local.selectedBankId) {
        mgraphics.set_source_rgba(options.active);
        mgraphics.set_line_width(2);
        mgraphics.rectangle(x + 1, y + 1, options.squareSize - 2, options.squareSize - 2);
        mgraphics.stroke();
    }
    if (selected) {
        mgraphics.set_source_rgba(options.selection);
        mgraphics.set_line_width(1);
        mgraphics.rectangle(x + 0.5, y + 0.5, options.squareSize - 1, options.squareSize - 1);
        mgraphics.stroke();
    }
    if (joinSelected) {
        mgraphics.set_source_rgba(options.joinSelection);
        mgraphics.set_line_width(2);
        mgraphics.rectangle(x + 2, y + 2, options.squareSize - 4, options.squareSize - 4);
        mgraphics.stroke();
    }
    mgraphics.set_source_rgba(interactive ? options.text : options.muted);
    mgraphics.set_font_size(8);
    var label = String(bank.id);
    var width = mgraphics.text_measure(label)[0];
    mgraphics.move_to(x + (options.squareSize - width) * 0.5, y + 13);
    mgraphics.show_text(label);
};

BankManager.prototype.Hash = function(value) {
    var hash = 0;
    for (var index = 0; index < value.length; index++) hash = ((hash << 5) - hash) + value.charCodeAt(index);
    return hash;
};

BankManager.prototype.Paint = function() {
    var width = mgraphics.size[0];
    var height = mgraphics.size[1];
    var options = BankManagerVisualOptions;
    mgraphics.set_source_rgba(options.background);
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();
    mgraphics.select_font_face("Ableton Sans", 0, 0);
    var rows = this.Rows();
    var y = options.padding;
    for (var rowIndex = 0; rowIndex < rows.length; rowIndex++) {
        var instance = rows[rowIndex];
        mgraphics.set_source_rgba(rowIndex === 0 ? options.active : options.row);
        mgraphics.rectangle(options.padding, y, width - options.padding * 2, options.rowHeight - 2);
        mgraphics.fill();
        mgraphics.set_source_rgba(options.text);
        mgraphics.set_font_size(9);
        mgraphics.move_to(options.padding + 5, y + 11);
        mgraphics.show_text(this.FitText(instance.label, width - options.padding * 2 - 10));
        var squareY = y + 13;
        var displayedBanks = [instance.systemBank].concat(instance.banks);
        for (var bankIndex = 0; bankIndex < displayedBanks.length; bankIndex++) {
            var x = options.padding + 5 + bankIndex * (options.squareSize + options.squareGap);
            var interactive = displayedBanks[bankIndex].id !== 0;
            this.DrawSquare(instance, displayedBanks[bankIndex], x, squareY, rowIndex === 0, interactive);
        }
        y += options.rowHeight;
    }
    var actionY = Math.max(y, height - options.actionHeight - options.padding);
    this.DrawActions(width, actionY);
};

BankManager.prototype.DrawActions = function(width, y) {
    var labels = ["JOIN", "COMMIT", "LINK", "UNLINK"];
    var actions = ["join", "commit", "link", "unlink"];
    var states = this.ActionStates();
    var buttonWidth = (width - BankManagerVisualOptions.padding * 2 - 3 * 4) / labels.length;
    for (var index = 0; index < labels.length; index++) {
        var x = BankManagerVisualOptions.padding + index * (buttonWidth + 4);
        var enabled = states[actions[index]];
        mgraphics.set_source_rgba(enabled ? BankManagerVisualOptions.row : BankManagerVisualOptions.disabledFill);
        mgraphics.rectangle(x, y, buttonWidth, BankManagerVisualOptions.actionHeight);
        mgraphics.fill();
        mgraphics.set_source_rgba(enabled ? BankManagerVisualOptions.border : BankManagerVisualOptions.disabledText);
        mgraphics.set_line_width(1);
        mgraphics.rectangle(x + .5, y + .5, buttonWidth - 1, BankManagerVisualOptions.actionHeight - 1);
        mgraphics.stroke();
        mgraphics.set_source_rgba(enabled ? BankManagerVisualOptions.text : BankManagerVisualOptions.disabledText);
        mgraphics.set_font_size(8);
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
    var rowIndex = Math.floor((y - options.padding) / options.rowHeight);
    if (rowIndex >= 0 && rowIndex < rows.length) {
        var bankOffset = x - (options.padding + 5);
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
    }
    var actionY = Math.max(options.padding + rows.length * options.rowHeight,
        mgraphics.size[1] - options.actionHeight - options.padding);
    if (y < actionY || y > actionY + options.actionHeight) return;
    var buttonWidth = (mgraphics.size[0] - options.padding * 2 - 12) / 4;
    var actionIndex = Math.floor((x - options.padding) / (buttonWidth + 4));
    var actions = ["join", "commit", "link", "unlink"];
    if (actionIndex >= 0 && actionIndex < actions.length) this.HandleAction(actions[actionIndex]);
};

var bankManager = new BankManager();

function inletassist(index) {
    assist(index === 0
        ? "Host EQ snapshots: snapshot 1 host eq <revision> <selected> <bypass> <solo> <bankCount> <banks...>"
        : "Global bank bus: bank.query, bank.announce, bank.leave, link.create, link.remove, link.join, link.parameter, link.filter_bypass");
}

function outletassist(index) {
    assist(index === 0
        ? "Host commands: eq.select_bank, eq.join_banks, eq.commit_hidden, eq.set_link"
        : "Global bank bus: bank.query, bank.announce, bank.leave, link.create, link.remove, link.join, link.parameter, link.filter_bypass");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function loadbang() {}
function initialize() { bankManager.Initialize(); }
function paint() { bankManager.Paint(); }
function onclick(x, y, button, cmd, shift, capslock, option, ctrl) { bankManager.Click(x, y, ctrl, cmd, shift); }
function snapshot() {
    if (inlet === 0) {
        var values = ["snapshot"].concat(arrayfromargs(arguments));
        if (String(values[3]) === "eq") bankManager.ParseEqSnapshot(values);
        else if (String(values[3]) === "definitions") bankManager.ParseDefinitions(values);
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
        else if (String(values[3]) === "device") bankManager.ParseDeviceSnapshot(values);
    }
}
function leave() {
    if (bankManager.instanceId) outlet(1, "bank.leave", bankManager.instanceId);
}
