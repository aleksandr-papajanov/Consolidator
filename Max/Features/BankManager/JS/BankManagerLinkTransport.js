function BankManagerLinkTransport(manager) {
    this.manager = manager;
    this.remoteHistoryOperations = {};
}

BankManagerLinkTransport.prototype.NextRevision = function(linkId) {
    return this.manager.linkRevisions.Next(linkId);
};

BankManagerLinkTransport.prototype.PublishEqPreview = function(
    bankId,
    filterId,
    parameterIndex,
    absoluteValue
) {
    outlet(2, "eq_preview", Number(bankId), Number(filterId),
        Number(parameterIndex), Number(absoluteValue));
};

BankManagerLinkTransport.prototype.HandleHistoryEvent = function(values) {
    var manager = this.manager;
    if (values.length < 4 || Number(values[0]) !== 1 ||
        String(values[1]) !== "host") return;
    var name = String(values[3]);
    if (name === "history.began" || name === "history.ended") {
        if (values.length !== 6) return;
        var operationId = String(values[4]);
        var linkId = String(values[5]);
        if (!operationId) return;
        var phase = name === "history.began" ? "begin" : "end";
        if (this.remoteHistoryOperations[operationId] === phase) {
            delete this.remoteHistoryOperations[operationId];
            return;
        }
        if (linkId === "-") return;
        outlet(1, name === "history.began" ? "link.history_begin" : "link.history_end",
            linkId, manager.instanceId, operationId);
        return;
    }
    if (name !== "history.restored" || values.length !== 7) return;
    var action = String(values[4]);
    var restoredOperationId = String(values[5]);
    var restoredLinkId = String(values[6]);
    if (!restoredOperationId || restoredLinkId === "-" ||
        (action !== "undo" && action !== "redo")) return;
    outlet(1, "link.history_restore", restoredLinkId, manager.instanceId,
        restoredOperationId, action);
};

BankManagerLinkTransport.prototype.ApplyHistoryBegin = function(values) {
    if (values.length !== 3) return;
    var manager = this.manager;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var operationId = String(values[2]);
    if (sourceId === manager.instanceId || !operationId || !manager.FindLocalLinkedBank(linkId)) return;
    this.remoteHistoryOperations[operationId] = "begin";
    manager.SendHostCommand("history.begin", [operationId]);
};

BankManagerLinkTransport.prototype.ApplyHistoryEnd = function(values) {
    if (values.length !== 3) return;
    var manager = this.manager;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var operationId = String(values[2]);
    if (sourceId === manager.instanceId || !operationId || !manager.FindLocalLinkedBank(linkId)) return;
    this.remoteHistoryOperations[operationId] = "end";
    manager.SendHostCommand("history.end", [operationId]);
};

BankManagerLinkTransport.prototype.ApplyHistoryRestore = function(values) {
    if (values.length !== 4) return;
    var manager = this.manager;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var operationId = String(values[2]);
    var action = String(values[3]);
    if (sourceId === manager.instanceId || !operationId ||
        (action !== "undo" && action !== "redo") || !manager.FindLocalLinkedBank(linkId)) return;
    manager.SendHostCommand("history.restore", [operationId, action]);
};

BankManagerLinkTransport.prototype.PublishProcessorPreview = function(
    device,
    parameter,
    absoluteValue
) {
    outlet(2, "processor_preview", String(device), String(parameter),
        Number(absoluteValue));
};

BankManagerLinkTransport.prototype.ApplyFilterDeltaToModel = function(
    update,
    skipInstanceId
) {
    var manager = this.manager;
    var members = manager.LinkMembers(update.linkId);
    var definition = (manager.filterDefinitions[update.filterId] || [])
        [update.parameterIndex];
    for (var index = 0; index < members.length; ++index) {
        if (members[index].instance.id === skipInstanceId) continue;
        var filter = members[index].bank.filters[update.filterId];
        var normalized = filter
            ? BankManagerMath.Normalize(filter.values[update.parameterIndex], definition)
            : NaN;
        if (isFinite(normalized)) {
            filter.values[update.parameterIndex] = BankManagerMath.Denormalize(
                normalized + update.delta,
                definition);
        }
    }
};

BankManagerLinkTransport.prototype.PublishFilterBypass = function(
    linkId,
    filterId,
    bypass
) {
    var manager = this.manager;
    var members = manager.LinkMembers(linkId);
    for (var index = 0; index < members.length; ++index) {
        var filter = members[index].bank.filters[filterId];
        if (filter) filter.bypass = bypass !== 0;
    }
    outlet(1, "link.filter_bypass", linkId, manager.instanceId,
        this.NextRevision(linkId), filterId, bypass);
};

BankManagerLinkTransport.prototype.PublishAnnouncement = function() {
    var manager = this.manager;
    if (!manager.instanceId || manager.local.banks.length !== 6) return false;
    var stateParts = [manager.instanceId, manager.local.label, manager.local.trackOrder,
        manager.local.selectedBankId, manager.local.systemBank.occupied ? 1 : 0];
    for (var stateIndex = 0; stateIndex < manager.local.banks.length; ++stateIndex) {
        stateParts.push(manager.local.banks[stateIndex].occupied ? 1 : 0);
        stateParts.push(manager.local.banks[stateIndex].linkId || "-");
    }
    var state = stateParts.join("|");
    if (state === manager.lastAnnouncementState) return false;

    var fields = [manager.instanceId, manager.local.label, manager.local.trackOrder,
        manager.local.revision, manager.local.selectedBankId,
        manager.local.systemBank.occupied ? 1 : 0];
    for (var bankIndex = 0; bankIndex < manager.local.banks.length; ++bankIndex) {
        var bank = manager.local.banks[bankIndex];
        fields.push(bank.id, bank.occupied ? 1 : 0, bank.linkId || "-");
    }
    manager.lastAnnouncementState = state;
    outlet(1, "bank.announce", fields);
    return true;
};

BankManagerLinkTransport.prototype.PublishLinkedState = function(linkIds) {
    var manager = this.manager;
    for (var bankIndex = 0; bankIndex < manager.local.banks.length; ++bankIndex) {
        var bank = manager.local.banks[bankIndex];
        if (!bank.linkId || (linkIds && !linkIds[bank.linkId])) continue;
        var fields = [bank.linkId, manager.instanceId, manager.local.revision, bank.id];
        var filters = [];
        for (var filterId in bank.filters) {
            if (!bank.filters.hasOwnProperty(filterId)) continue;
            var filter = bank.filters[filterId];
            filters.push({ id: Number(filterId), bypass: filter.bypass, values: filter.values });
        }
        fields.push(filters.length);
        for (var filterIndex = 0; filterIndex < filters.length; ++filterIndex) {
            var filterState = filters[filterIndex];
            fields.push(filterState.id, filterState.bypass ? 1 : 0,
                filterState.values.length);
            for (var valueIndex = 0;
                 valueIndex < filterState.values.length;
                 ++valueIndex) {
                fields.push(filterState.values[valueIndex]);
            }
        }
        var processorValues = [];
        for (var device in manager.local.processors) {
            if (!manager.local.processors.hasOwnProperty(device)) continue;
            var processor = manager.local.processors[device];
            for (var parameter in processor.values) {
                if (!processor.values.hasOwnProperty(parameter) ||
                    !isFinite(processor.values[parameter])) continue;
                processorValues.push({ device: device, parameter: parameter,
                    value: processor.values[parameter] });
            }
        }
        fields.push(processorValues.length);
        for (var processorIndex = 0;
             processorIndex < processorValues.length;
             ++processorIndex) {
            var processorState = processorValues[processorIndex];
            fields.push(processorState.device, processorState.parameter,
                processorState.value);
        }
        outlet(1, "link.state", fields);
    }
};

BankManagerLinkTransport.prototype.ParseAnnouncement = function(values) {
    var manager = this.manager;
    if (values.length !== 24) return;
    var instanceId = String(values[0]);
    if (!instanceId || instanceId === manager.instanceId) return;
    var trackOrder = Number(values[2]);
    var revision = Number(values[3]);
    var selected = Number(values[4]);
    if (!isFinite(trackOrder) || !isFinite(revision) || selected < 1 || selected > 6) return;
    var peer = manager.peers[instanceId] || new InstanceSummary(instanceId, String(values[1]));
    if (revision < peer.revision) return;
    peer.label = String(values[1]);
    peer.trackOrder = trackOrder;
    peer.revision = revision;
    peer.selectedBankId = selected;
    peer.systemBank.occupied = Number(values[5]) !== 0;
    var position = 6;
    var linkTopologyChanged = false;
    for (var index = 0; index < 6; ++index) {
        var bankId = Number(values[position++]);
        var occupied = Number(values[position++]) !== 0;
        var linkId = manager.NormalizeLinkId(values[position++]);
        if (bankId !== index + 1) return;
        if (peer.banks[index].linkId !== linkId) {
            peer.banks[index].filters = {};
            linkTopologyChanged = true;
        }
        peer.banks[index].occupied = occupied;
        peer.banks[index].linkId = linkId;
    }
    manager.peers[instanceId] = peer;
    if (!linkTopologyChanged) return;
    manager.RebuildProcessorLinkGroups();
    if (manager.selection.focusedInstanceId === manager.instanceId) {
        manager.controlLinkSession = "";
        manager.RefreshControlLinkSession();
    }
};

BankManagerLinkTransport.prototype.ApplyLinkedState = function(values) {
    var manager = this.manager;
    if (values.length < 6) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var bankId = Number(values[3]);
    var filterCount = Number(values[4]);
    var peer = manager.peers[sourceId];
    if (!peer || sourceId === manager.instanceId || !isFinite(revision) ||
        !isFinite(bankId) || !isFinite(filterCount) || filterCount < 0) return;
    var bank = peer.banks[bankId - 1];
    if (!bank || bank.linkId !== linkId) return;
    var position = 5;
    var filters = {};
    for (var filterIndex = 0; filterIndex < filterCount; ++filterIndex) {
        if (position + 2 >= values.length) return;
        var filterId = Number(values[position++]);
        var bypass = Number(values[position++]) !== 0;
        var valueCount = Number(values[position++]);
        if (!isFinite(filterId) || !isFinite(valueCount) || valueCount < 0 ||
            position + valueCount > values.length) return;
        var filterValues = [];
        for (var valueIndex = 0; valueIndex < valueCount; ++valueIndex) {
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
    for (var processorIndex = 0;
         processorIndex < processorValueCount;
         ++processorIndex) {
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
            processor.values[processorParameter] =
                processors[processorDevice][processorParameter];
        }
    }
    if (manager.ActiveLinkId(manager.local) !== linkId) return;
    manager.controlLinkSession = "";
    manager.RefreshControlLinkSession();
};

BankManagerLinkTransport.prototype.ApplyProcessorDelta = function(values) {
    var manager = this.manager;
    if (values.length !== 6) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var device = String(values[3]);
    var parameter = String(values[4]);
    var delta = Number(values[5]);
    if (sourceId === manager.instanceId || !isFinite(revision) || !isFinite(delta)) return;
    var group = manager.ProcessorLinkGroup(linkId, device);
    var range = manager.processorRanges[device] && manager.processorRanges[device][parameter];
    if (!group || !range || !manager.linkRevisions.AcceptUpdate(linkId, sourceId, revision)) return;
    group.ApplyDelta(sourceId, parameter, delta, false, range);
    var processor = manager.local.processors[device];
    if (!processor || !manager.HasLink(manager.local, linkId)) return;
    this.PublishProcessorPreview(device, parameter, processor.values[parameter]);
    if (parameter.indexOf("detector.") === 0) {
        manager.PublishLinkedDetectorPreviews(linkId, true);
    }
    manager.SendProcessorValue(device, parameter, processor.values[parameter]);
};

BankManagerLinkTransport.prototype.ApplyDetectorReset = function(values) {
    var manager = this.manager;
    if (values.length !== 5) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var device = String(values[3]);
    var filterId = Number(values[4]);
    if (sourceId === manager.instanceId ||
        (device !== "compressor" && device !== "saturator") ||
        !isFinite(filterId) || !manager.linkRevisions.AcceptOperation(
            linkId, sourceId, revision)) return;
    if (!manager.HasLink(manager.local, linkId)) return;
    manager.ResetDetectorModels(linkId, device, filterId);
    manager.SendDetectorReset(device, filterId);
    manager.PublishLinkedDetectorPreviews(linkId, true);
};

BankManagerLinkTransport.prototype.RemovePeer = function(values) {
    var manager = this.manager;
    if (values.length !== 1) return;
    var instanceId = String(values[0]);
    if (!instanceId || instanceId === manager.instanceId) return;
    delete manager.peers[instanceId];
    if (manager.selection.focusedInstanceId === instanceId) {
        manager.SetFocusedBank(manager.local, manager.local.selectedBankId);
    }
    manager.RebuildProcessorLinkGroups();
    manager.controlLinkSession = "";
    manager.RefreshControlLinkSession();
};

BankManagerLinkTransport.prototype.ApplyFilterDelta = function(values) {
    var manager = this.manager;
    if (values.length !== 6) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var filterId = Number(values[3]);
    var parameterIndex = Number(values[4]);
    var delta = Number(values[5]);
    if (sourceId === manager.instanceId || !isFinite(revision) ||
        !isFinite(filterId) || !isFinite(parameterIndex) || !isFinite(delta)) return;
    var bank = manager.FindLocalLinkedBank(linkId);
    var filter = bank && bank.filters[filterId];
    if (!filter || !isFinite(filter.values[parameterIndex]) ||
        !manager.linkRevisions.AcceptUpdate(linkId, sourceId, revision)) return;
    var update = { linkId: linkId, filterId: filterId,
        parameterIndex: parameterIndex, delta: delta };
    this.ApplyFilterDeltaToModel(update, "");
    this.PublishEqPreview(bank.id, filterId, parameterIndex,
        filter.values[parameterIndex]);
    manager.PublishChangedFilterPreviews(linkId, filterId);
    manager.SendHostCommand("eq.set_parameter_index", [
        bank.id, filterId, parameterIndex, filter.values[parameterIndex]
    ]);
};

BankManagerLinkTransport.prototype.ApplyFilterBypass = function(values) {
    var manager = this.manager;
    if (values.length !== 5) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var filterId = Number(values[3]);
    var bypass = Number(values[4]) ? 1 : 0;
    if (sourceId === manager.instanceId || !isFinite(revision) ||
        !isFinite(filterId)) return;
    var localBank = manager.FindLocalLinkedBank(linkId);
    if (!localBank || !manager.linkRevisions.AcceptUpdate(linkId, sourceId, revision)) return;
    var members = manager.LinkMembers(linkId);
    for (var index = 0; index < members.length; ++index) {
        var filter = members[index].bank.filters[filterId];
        if (filter) filter.bypass = bypass !== 0;
    }
    manager.PublishLinkedFilterPreviews(linkId, true);
    manager.SendHostCommand("eq.set_bypass", [localBank.id, filterId, bypass]);
};

BankManagerLinkTransport.prototype.ApplyFilterReset = function(values) {
    var manager = this.manager;
    if (values.length !== 4) return;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var filterId = Number(values[3]);
    if (sourceId === manager.instanceId || !isFinite(filterId) ||
        !manager.linkRevisions.AcceptOperation(linkId, sourceId, revision)) return;
    var bank = manager.FindLocalLinkedBank(linkId);
    if (!bank) return;
    manager.ResetLinkedFilterModels(linkId, filterId);
    manager.PublishLinkedFilterPreviews(linkId, true);
    manager.SendHostCommand("eq.reset_filter", [bank.id, filterId]);
};

BankManagerLinkTransport.prototype.HandleEqGesture = function(values) {
    var manager = this.manager;
    if (values.length !== 4) return;
    var bankId = Number(values[0]);
    var filterId = Number(values[1]);
    var parameterName = String(values[2]);
    var absolute = Number(values[3]);
    var parameters = manager.filterDefinitions[filterId] || [];
    for (var parameterIndex = 0; parameterIndex < parameters.length; ++parameterIndex) {
        if (parameters[parameterIndex].name !== parameterName) continue;
        var normalized = BankManagerMath.Normalize(absolute, parameters[parameterIndex]);
        var bank = manager.LocalBank(bankId);
        var filter = bank && bank.filters[filterId];
        if (!bank || !filter || !isFinite(normalized)) return;
        this.PublishEqPreview(bankId, filterId, parameterIndex, absolute);
        if (!bank.linkId) return;
        var previousNormalized = BankManagerMath.Normalize(
            filter.values[parameterIndex], parameters[parameterIndex]);
        var delta = normalized - previousNormalized;
        if (!isFinite(previousNormalized) || !isFinite(delta) || !delta) return;
        var update = { linkId: bank.linkId, bankId: bankId, filterId: filterId,
            parameterIndex: parameterIndex, delta: delta };
        filter.values[parameterIndex] = absolute;
        this.ApplyFilterDeltaToModel(update, manager.instanceId);
        manager.PublishChangedFilterPreviews(update.linkId, filterId);
        outlet(1, "link.filter_delta", update.linkId, manager.instanceId,
            this.NextRevision(update.linkId), filterId, parameterIndex, delta);
        return;
    }
};

BankManagerLinkTransport.prototype.HandleEqPreview = function(values) {
    var manager = this.manager;
    if (values.length !== 4) return;
    var absoluteValue = Number(values[3]);
    if (!isFinite(absoluteValue)) return;
    var filterId = Number(values[1]);
    var parameterName = String(values[2]);
    var parameters = manager.filterDefinitions[filterId] || [];
    for (var parameterIndex = 0; parameterIndex < parameters.length; ++parameterIndex) {
        if (parameters[parameterIndex].name !== parameterName) continue;
        this.PublishEqPreview(values[0], filterId, parameterIndex, absoluteValue);
        return;
    }
};

BankManagerLinkTransport.prototype.HandleProcessorGesture = function(values) {
    var manager = this.manager;
    if (values.length !== 3) return;
    var device = String(values[0]);
    var parameter = String(values[1]);
    var normalized = Number(values[2]);
    var processor = manager.local.processors[device];
    var range = manager.processorRanges[device] && manager.processorRanges[device][parameter];
    var linkId = manager.ActiveLinkId(manager.local);
    var group = linkId ? manager.ProcessorLinkGroup(linkId, device) : null;
    if (!processor || !range || !group || !isFinite(normalized)) return;
    var previousNormalized = BankManagerMath.Normalize(processor.values[parameter], range);
    var delta = normalized - previousNormalized;
    if (!isFinite(previousNormalized) || !isFinite(delta) || !delta) return;
    processor.values[parameter] = BankManagerMath.Denormalize(normalized, range);
    group.ApplyDelta(manager.instanceId, parameter, delta, true, range);
    this.PublishProcessorPreview(device, parameter, processor.values[parameter]);
    if (parameter.indexOf("detector.") === 0) {
        manager.PublishLinkedDetectorPreviews(linkId, true);
    }
    outlet(1, "link.processor_delta", linkId, manager.instanceId,
        this.NextRevision(linkId), device, parameter, delta);
};

BankManagerLinkTransport.prototype.HandleDetectorReset = function(values) {
    var manager = this.manager;
    if (values.length !== 2) return;
    var device = String(values[0]);
    var filterId = Number(values[1]);
    if ((device !== "compressor" && device !== "saturator") ||
        !isFinite(filterId) || filterId < 1 || filterId > 2) return;
    var linkId = manager.ActiveLinkId(manager.local);
    var group = linkId ? manager.ProcessorLinkGroup(linkId, device) : null;
    var isLinked = Boolean(group && Object.keys(group.members).length >= 2);
    manager.ResetDetectorModels(isLinked ? linkId : "", device, filterId);
    manager.SendDetectorReset(device, filterId);
    manager.PublishLinkedDetectorPreviews(linkId, isLinked);
    if (!isLinked) return;
    outlet(1, "link.processor_detector_reset", linkId, manager.instanceId,
        this.NextRevision(linkId), device, filterId);
};

BankManagerLinkTransport.prototype.StartProcessorMatch = function(device, operation) {
    var manager = this.manager;
    var linkId = manager.ActiveLinkId(manager.local);
    outlet(2, "processor_match_operation", String(device), String(operation));
    if (!linkId) return;
    outlet(1, "link.processor_match", linkId, manager.instanceId,
        this.NextRevision(linkId), String(device), String(operation));
};

BankManagerLinkTransport.prototype.ApplyProcessorMatch = function(values) {
    if (values.length !== 5) return;
    var manager = this.manager;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var device = String(values[3]);
    var operation = String(values[4]);
    if (sourceId === manager.instanceId || !isFinite(revision) ||
        (operation !== "onset" && operation !== "level") ||
        !manager.linkRevisions.AcceptOperation(linkId, sourceId, revision) ||
        !manager.FindLocalLinkedBank(linkId)) return;
    outlet(2, "processor_match_operation", device, operation);
};

BankManagerLinkTransport.prototype.StartProcessorBypass = function(device, bypass) {
    var manager = this.manager;
    var linkId = manager.ActiveLinkId(manager.local);
    manager.SendProcessorBypass(device, bypass);
    outlet(2, "processor_bypass_operation", String(device), Number(bypass));
    if (!linkId) return;
    outlet(1, "link.processor_bypass", linkId, manager.instanceId,
        this.NextRevision(linkId), String(device), Number(bypass));
};

BankManagerLinkTransport.prototype.ApplyProcessorBypass = function(values) {
    if (values.length !== 5) return;
    var manager = this.manager;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var device = String(values[3]);
    var bypass = Number(values[4]);
    if (sourceId === manager.instanceId || !isFinite(revision) ||
        (device !== "compressor" && device !== "saturator") ||
        (bypass !== 0 && bypass !== 1) ||
        !manager.linkRevisions.AcceptOperation(linkId, sourceId, revision) ||
        !manager.FindLocalLinkedBank(linkId)) return;
    manager.SendProcessorBypass(device, bypass);
    outlet(2, "processor_bypass_operation", device, bypass);
};

BankManagerLinkTransport.prototype.HandleGlobal = function(name, values) {
    var manager = this.manager;
    var shouldRedraw = name !== "link.filter_delta" &&
        name !== "link.processor_delta" && name !== "link.state";
    if (name === "bank.query") {
        if (String(values[0]) !== manager.instanceId) {
            manager.lastAnnouncementState = "";
            this.PublishAnnouncement();
            this.PublishLinkedState();
        }
    } else if (name === "bank.announce") {
        this.ParseAnnouncement(values);
    } else if (name === "bank.reset_all") {
        if (values.length === 1) {
            manager.ResetAllBankModels();
            manager.SendHostCommand("eq.reset_all", []);
        }
    } else if (name === "link.state") {
        this.ApplyLinkedState(values);
    } else if (name === "bank.leave") {
        this.RemovePeer(values);
    } else if (name === "link.assign") {
        manager.ApplyLinkAssignment(values);
    } else if (name === "link.detach") {
        manager.ApplyLinkDetachment(values);
    } else if (name === "link.operation") {
        manager.ApplyLinkOperation(values);
    } else if (name === "link.filter_delta") {
        this.ApplyFilterDelta(values);
    } else if (name === "link.filter_bypass") {
        this.ApplyFilterBypass(values);
    } else if (name === "link.filter_reset") {
        this.ApplyFilterReset(values);
    } else if (name === "link.processor_delta") {
        this.ApplyProcessorDelta(values);
    } else if (name === "link.processor_match") {
        this.ApplyProcessorMatch(values);
    } else if (name === "link.processor_bypass") {
        this.ApplyProcessorBypass(values);
    } else if (name === "link.processor_detector_reset") {
        this.ApplyDetectorReset(values);
    } else if (name === "link.history_begin") {
        this.ApplyHistoryBegin(values);
    } else if (name === "link.history_end") {
        this.ApplyHistoryEnd(values);
    } else if (name === "link.history_restore") {
        this.ApplyHistoryRestore(values);
    }
    if (shouldRedraw) mgraphics.redraw();
};
