function BankManagerOperations(manager) {
    this.manager = manager;
    this.remoteHistoryOperations = {};
}

BankManagerOperations.prototype.NextRevision = function(linkId) {
    return this.manager.linkRevisions.Next(linkId);
};

BankManagerOperations.prototype.HandleHistoryEvent = function(values) {
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

BankManagerOperations.prototype.ApplyHistoryBegin = function(values) {
    if (values.length !== 3) return;
    var manager = this.manager;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var operationId = String(values[2]);
    if (sourceId === manager.instanceId || !operationId ||
        !manager.FindLocalLinkedBank(linkId)) return;
    this.remoteHistoryOperations[operationId] = "begin";
    manager.SendHostCommand("history.begin", [operationId]);
};

BankManagerOperations.prototype.ApplyHistoryEnd = function(values) {
    if (values.length !== 3) return;
    var manager = this.manager;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var operationId = String(values[2]);
    if (sourceId === manager.instanceId || !operationId ||
        !manager.FindLocalLinkedBank(linkId)) return;
    this.remoteHistoryOperations[operationId] = "end";
    manager.SendHostCommand("history.end", [operationId]);
};

BankManagerOperations.prototype.ApplyHistoryRestore = function(values) {
    if (values.length !== 4) return;
    var manager = this.manager;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var operationId = String(values[2]);
    var action = String(values[3]);
    if (sourceId === manager.instanceId || !operationId ||
        (action !== "undo" && action !== "redo") ||
        !manager.FindLocalLinkedBank(linkId)) return;
    manager.SendHostCommand("history.restore", [operationId, action]);
};

BankManagerOperations.prototype.Assign = function(values) {
    if (values.length !== 3) return;
    var manager = this.manager;
    var linkId = String(values[0]);
    var instanceId = String(values[1]);
    var bankId = Number(values[2]);
    if (instanceId !== manager.instanceId ||
        manager.EditableLinkIds().indexOf(linkId) < 0 || bankId < 2 || bankId > 5) return;
    manager.QueueLinkMutation(manager.local, manager.LocalBank(bankId), linkId);
};

BankManagerOperations.prototype.Detach = function(values) {
    if (values.length !== 3) return;
    var manager = this.manager;
    var linkId = String(values[0]);
    var instanceId = String(values[1]);
    var bankId = Number(values[2]);
    if (instanceId !== manager.instanceId || bankId < 2 || bankId > 5) return;
    var bank = manager.LocalBank(bankId);
    if (bank && bank.linkId === linkId) manager.QueueLinkMutation(manager.local, bank, "");
};

BankManagerOperations.prototype.Execute = function(action, bypass) {
    var manager = this.manager;
    var instance = manager.FocusedInstance();
    var bank = manager.FocusedBank();
    if (!bank) return;
    if (!bank.linkId || instance.id !== manager.instanceId) {
        this.Apply(action, bank.id, bypass);
        return;
    }
    this.Apply(action, bank.id, bypass);
    outlet(1, "link.operation", bank.linkId, instance.id,
        manager.NextLinkRevision(bank.linkId), action,
        bypass === undefined ? -1 : bypass);
};

BankManagerOperations.prototype.Apply = function(action, bankId, bypass) {
    var manager = this.manager;
    if (action === "join") manager.SendHostCommand("eq.join_banks", [1, bankId]);
    else if (action === "commit") manager.SendHostCommand("eq.commit_all", []);
    else if (action === "reset") manager.SendHostCommand("eq.reset", [bankId]);
    else if (action === "bypass") {
        manager.SendHostCommand("eq.set_chain_bypass", [Number(bypass) !== 0 ? 1 : 0]);
    }
};

BankManagerOperations.prototype.ApplyLinked = function(values) {
    if (values.length !== 5) return;
    var manager = this.manager;
    var linkId = String(values[0]);
    var sourceId = String(values[1]);
    var revision = Number(values[2]);
    var action = String(values[3]);
    var bypass = Number(values[4]);
    if (sourceId === manager.instanceId || !isFinite(revision) ||
        !manager.linkRevisions.AcceptOperation(linkId, sourceId, revision)) return;
    var bank = manager.FindLocalLinkedBank(linkId);
    if (!bank) return;
    this.Apply(action, bank.id, bypass);
};

BankManagerOperations.prototype.ResetFilter = function(bankId, filterId) {
    var manager = this.manager;
    var instance = manager.FocusedInstance();
    var bank = instance && instance.banks[Number(bankId) - 1];
    if (!bank || !isFinite(filterId)) return;
    if (!bank.linkId) {
        manager.SendHostCommand("eq.reset_filter", [bank.id, filterId]);
        return;
    }
    manager.SendHostCommand("eq.reset_filter", [bank.id, filterId]);
    outlet(1, "link.filter_reset", bank.linkId, manager.instanceId,
        manager.NextLinkRevision(bank.linkId), filterId);
};

BankManagerOperations.prototype.PublishFilterBypass = function(linkId, filterId, bypass) {
    var manager = this.manager;
    outlet(1, "link.filter_bypass", linkId, manager.instanceId,
        this.NextRevision(linkId), filterId, bypass);
};

BankManagerOperations.prototype.ApplyDetectorReset = function(values) {
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
    manager.SendDetectorReset(device, filterId);
};

BankManagerOperations.prototype.ApplyFilterBypass = function(values) {
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
    manager.SendHostCommand("eq.set_bypass", [localBank.id, filterId, bypass]);
};

BankManagerOperations.prototype.ApplyFilterReset = function(values) {
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
    manager.SendHostCommand("eq.reset_filter", [bank.id, filterId]);
};

BankManagerOperations.prototype.HandleDetectorReset = function(values) {
    if (values.length !== 2) return;
    var manager = this.manager;
    var device = String(values[0]);
    var filterId = Number(values[1]);
    if ((device !== "compressor" && device !== "saturator") ||
        !isFinite(filterId) || filterId < 1 || filterId > 2) return;
    var linkId = manager.ActiveLinkId(manager.local);
    manager.SendDetectorReset(device, filterId);
    if (!linkId) return;
    outlet(1, "link.processor_detector_reset", linkId, manager.instanceId,
        this.NextRevision(linkId), device, filterId);
};

BankManagerOperations.prototype.StartProcessorMatch = function(device, operation) {
    var manager = this.manager;
    var linkId = manager.ActiveLinkId(manager.local);
    outlet(2, "processor_match_operation", String(device), String(operation));
    if (!linkId) return;
    outlet(1, "link.processor_match", linkId, manager.instanceId,
        this.NextRevision(linkId), String(device), String(operation));
};

BankManagerOperations.prototype.ApplyProcessorMatch = function(values) {
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

BankManagerOperations.prototype.StartProcessorBypass = function(device, bypass) {
    var manager = this.manager;
    var linkId = manager.ActiveLinkId(manager.local);
    manager.SendProcessorBypass(device, bypass);
    outlet(2, "processor_bypass_operation", String(device), Number(bypass));
    if (!linkId) return;
    outlet(1, "link.processor_bypass", linkId, manager.instanceId,
        this.NextRevision(linkId), String(device), Number(bypass));
};

BankManagerOperations.prototype.ApplyProcessorBypass = function(values) {
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

BankManagerOperations.prototype.HandleGlobal = function(name, values) {
    var manager = this.manager;
    if (name === "coordinator.changed") {
        outlet(0, "coordinator_refresh");
        manager.SynchronizeCoordinator();
    } else if (name === "bank.reset_all") {
        if (values.length === 1) {
            manager.ResetAllBankModels();
            manager.SendHostCommand("eq.reset_all", []);
        }
    } else if (name === "link.assign") {
        this.Assign(values);
    } else if (name === "link.detach") {
        this.Detach(values);
    } else if (name === "link.operation") {
        this.ApplyLinked(values);
    } else if (name === "link.filter_bypass") {
        this.ApplyFilterBypass(values);
    } else if (name === "link.filter_reset") {
        this.ApplyFilterReset(values);
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
    } else {
        return;
    }
    mgraphics.redraw();
};
