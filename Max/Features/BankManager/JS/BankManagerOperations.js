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
