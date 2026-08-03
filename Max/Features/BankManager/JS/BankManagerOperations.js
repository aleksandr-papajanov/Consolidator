function BankManagerOperations(manager) {
    this.manager = manager;
    this.remoteHistoryOperations = {};
}

BankManagerOperations.prototype.NextRevision = function(linkId) {
    return this.manager.linkRevisions.Next(linkId);
};

BankManagerOperations.prototype.HandleHistoryEvent = function(values) {
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
    var bank = manager.FocusedBank();
    if (!bank) return;
    this.Apply(action, bank.id, bypass);
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

BankManagerOperations.prototype.ResetFilter = function(bankId, filterId) {
    var manager = this.manager;
    var instance = manager.FocusedInstance();
    var bank = instance && instance.banks[Number(bankId) - 1];
    if (!bank || !isFinite(filterId)) return;
    manager.SendHostCommand("eq.reset_filter", [bank.id, filterId]);
};

BankManagerOperations.prototype.HandleDetectorReset = function(values) {
    if (values.length !== 2) return;
    var manager = this.manager;
    var device = String(values[0]);
    var filterId = Number(values[1]);
    if ((device !== "compressor" && device !== "saturator") ||
        !isFinite(filterId) || filterId < 1 || filterId > 2) return;
    manager.SendDetectorReset(device, filterId);
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
    manager.SendProcessorBypass(device, bypass);
    outlet(2, "processor_bypass_operation", String(device), Number(bypass));
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
