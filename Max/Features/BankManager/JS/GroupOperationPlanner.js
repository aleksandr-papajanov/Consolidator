function GroupOperationPlanner(manager) {
    this.manager = manager;
}

GroupOperationPlanner.prototype.IsEditableBank = function(bank) {
    return Boolean(bank && bank.id >= 2 && bank.id <= 5);
};

GroupOperationPlanner.prototype.IsSelected = function(instance, bank) {
    return this.manager.selection.IsEditSelected(instance, bank);
};

GroupOperationPlanner.prototype.EditBankMembership = function(instance, bank, extend) {
    if (!this.manager.linkEditingEnabled || !this.IsEditableBank(bank)) return;
    this.manager.selection.ToggleEditBank(instance, bank, extend);
};

GroupOperationPlanner.prototype.Selection = function() {
    return this.manager.selection.EditSelection();
};

GroupOperationPlanner.prototype.CanAssignBank = function(instance, bank, linkId, reservedInstances) {
    var manager = this.manager;
    if (!instance || !this.IsEditableBank(bank) || bank.linkId ||
        manager.EditableLinkIds().indexOf(linkId) < 0) return false;
    if (reservedInstances && reservedInstances[instance.id]) return false;
    for (var index = 0; index < instance.banks.length; ++index) {
        var candidate = instance.banks[index];
        if (candidate.id !== bank.id && candidate.linkId === linkId) return false;
    }
    return true;
};

GroupOperationPlanner.prototype.CanAssignSelection = function(linkId) {
    var manager = this.manager;
    var members = this.Selection();
    if (members.length === 0 || manager.EditableLinkIds().indexOf(linkId) < 0) return false;
    var reservedInstances = {};
    for (var index = 0; index < members.length; ++index) {
        var member = members[index];
        if (!this.CanAssignBank(member.instance, member.bank, linkId, reservedInstances)) {
            return false;
        }
        reservedInstances[member.instance.id] = true;
    }
    return true;
};

GroupOperationPlanner.prototype.SelectionForLink = function(linkId) {
    return this.Selection().filter(function(member) {
        return member.bank.linkId === linkId;
    });
};

GroupOperationPlanner.prototype.CanDetachSelection = function(linkId) {
    var members = this.Selection();
    if (members.length === 0) return false;
    for (var index = 0; index < members.length; ++index) {
        if (members[index].bank.linkId !== linkId) return false;
    }
    return true;
};

GroupOperationPlanner.prototype.CanApplySelection = function(linkId) {
    return this.CanDetachSelection(linkId) || this.CanAssignSelection(linkId);
};

GroupOperationPlanner.prototype.HasSelectionInLink = function(linkId) {
    return this.CanDetachSelection(linkId);
};

GroupOperationPlanner.prototype.AssignSelection = function(linkId) {
    var manager = this.manager;
    if (!manager.linkEditingEnabled || !this.CanAssignSelection(linkId)) return;
    var members = this.Selection();
    for (var index = 0; index < members.length; ++index) {
        var member = members[index];
        if (member.instance.id === manager.instanceId) {
            manager.SendHostCommand("eq.set_link", [member.bank.id, linkId]);
        } else {
            outlet(1, "link.assign", linkId, member.instance.id, member.bank.id);
        }
    }
};

GroupOperationPlanner.prototype.DetachSelection = function(linkId) {
    var manager = this.manager;
    if (!this.CanDetachSelection(linkId)) return false;
    var members = this.SelectionForLink(linkId);
    for (var index = 0; index < members.length; ++index) {
        var member = members[index];
        if (member.instance.id === manager.instanceId) {
            manager.SendHostCommand("eq.set_link", [member.bank.id, "-"]);
        } else {
            outlet(1, "link.detach", linkId, member.instance.id, member.bank.id);
        }
    }
    return true;
};

GroupOperationPlanner.prototype.ApplySelection = function(linkId) {
    if (!this.manager.linkEditingEnabled) return;
    if (this.DetachSelection(linkId)) return;
    this.AssignSelection(linkId);
};
