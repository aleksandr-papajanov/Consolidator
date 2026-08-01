function BankManagerOperations(manager) {
    this.manager = manager;
}

BankManagerOperations.prototype.Assign = function(values) {
    if (values.length !== 3) return;
    var manager = this.manager;
    var linkId = String(values[0]);
    var instanceId = String(values[1]);
    var bankId = Number(values[2]);
    if (instanceId !== manager.instanceId ||
        manager.EditableLinkIds().indexOf(linkId) < 0 || bankId < 2 || bankId > 5) return;
    manager.SendHostCommand("eq.set_link", [bankId, linkId]);
};

BankManagerOperations.prototype.Detach = function(values) {
    if (values.length !== 3) return;
    var manager = this.manager;
    var linkId = String(values[0]);
    var instanceId = String(values[1]);
    var bankId = Number(values[2]);
    if (instanceId !== manager.instanceId || bankId < 2 || bankId > 5) return;
    var bank = manager.LocalBank(bankId);
    if (bank && bank.linkId === linkId) manager.SendHostCommand("eq.set_link", [bankId, "-"]);
};

BankManagerOperations.prototype.Execute = function(action, bypass) {
    var manager = this.manager;
    var bank = manager.ActiveBank(manager.local);
    if (!bank) return;
    if (!bank.linkId) {
        this.Apply(action, bank.id, bypass);
        return;
    }
    if (action === "join" || action === "commit") manager.pendingLinkedStatePublish = true;
    if (action === "reset") {
        manager.ResetLinkedBankModels(bank.linkId);
        manager.PublishLinkedFilterPreviews(bank.linkId, true);
    }
    this.Apply(action, bank.id, bypass);
    outlet(1, "link.operation", bank.linkId, manager.instanceId,
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
        !manager.AcceptIncomingOperationRevision(linkId, sourceId, revision)) return;
    var bank = manager.FindLocalLinkedBank(linkId);
    if (!bank) return;
    if (action === "reset") {
        manager.ResetLinkedBankModels(linkId);
        manager.PublishLinkedFilterPreviews(linkId, true);
    } else if (action === "join" || action === "commit") {
        manager.pendingLinkedStatePublish = true;
    }
    this.Apply(action, bank.id, bypass);
};

BankManagerOperations.prototype.ResetFilter = function(bankId, filterId) {
    var manager = this.manager;
    var bank = manager.LocalBank(bankId);
    if (!bank || !isFinite(filterId)) return;
    if (!bank.linkId) {
        this.ResetFilterModel(bank, filterId);
        manager.SendHostCommand("eq.reset_filter", [bank.id, filterId]);
        return;
    }
    this.ResetLinkedFilterModels(bank.linkId, filterId);
    manager.PublishLinkedFilterPreviews(bank.linkId, true);
    manager.SendHostCommand("eq.reset_filter", [bank.id, filterId]);
    outlet(1, "link.filter_reset", bank.linkId, manager.instanceId,
        manager.NextLinkRevision(bank.linkId), filterId);
};

BankManagerOperations.prototype.ResetLinkedFilterModels = function(linkId, filterId) {
    var manager = this.manager;
    var members = manager.LinkMembers(linkId);
    for (var index = 0; index < members.length; ++index) {
        this.ResetFilterModel(members[index].bank, filterId);
    }
};

BankManagerOperations.prototype.ResetFilterModel = function(bank, filterId) {
    var manager = this.manager;
    var filter = bank && bank.filters[filterId];
    var parameters = manager.filterDefinitions[filterId];
    if (!filter || !parameters) return;
    var values = [];
    for (var index = 0; index < parameters.length; ++index) {
        values.push(Number(parameters[index].defaultValue));
    }
    filter.values = values;
    filter.bypass = Boolean(manager.filterDefaultBypass[filterId]);
};

BankManagerOperations.prototype.ResetAllModels = function() {
    var manager = this.manager;
    var instances = [manager.local];
    for (var instanceId in manager.peers) {
        if (manager.peers.hasOwnProperty(instanceId)) {
            instances.push(manager.peers[instanceId]);
        }
    }
    for (var instanceIndex = 0; instanceIndex < instances.length; ++instanceIndex) {
        var instance = instances[instanceIndex];
        instance.systemBank.filters = {};
        instance.systemBank.occupied = false;
        for (var bankIndex = 0; bankIndex < instance.banks.length; ++bankIndex) {
            this.ResetBankModel(instance.banks[bankIndex]);
        }
    }
};

BankManagerOperations.prototype.ResetBankModel = function(bank) {
    var manager = this.manager;
    bank.filters = {};
    bank.occupied = false;
    for (var filterId in manager.filterDefinitions) {
        if (!manager.filterDefinitions.hasOwnProperty(filterId)) continue;
        var parameters = manager.filterDefinitions[filterId];
        var values = [];
        for (var parameterIndex = 0;
             parameterIndex < parameters.length;
             ++parameterIndex) {
            values.push(Number(parameters[parameterIndex].defaultValue));
        }
        bank.filters[filterId] = {
            bypass: Boolean(manager.filterDefaultBypass[filterId]),
            values: values
        };
    }
};
