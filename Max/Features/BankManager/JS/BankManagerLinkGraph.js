function BankManagerLinkGraph(manager) {
    this.manager = manager;
    this.processorGroups = {};
}

BankManagerLinkGraph.prototype.Rebuild = function() {
    var manager = this.manager;
    var groups = {};
    var rows = manager.Rows();
    for (var rowIndex = 0; rowIndex < rows.length; ++rowIndex) {
        var instance = rows[rowIndex];
        var linkIds = {};
        for (var bankIndex = 0; bankIndex < instance.banks.length; ++bankIndex) {
            var linkId = instance.banks[bankIndex].linkId;
            if (linkId) linkIds[linkId] = true;
        }
        for (var linkId in linkIds) {
            if (!linkIds.hasOwnProperty(linkId)) continue;
            for (var device in instance.processors) {
                if (!instance.processors.hasOwnProperty(device)) continue;
                var key = linkId + ":" + device;
                if (!groups[key]) groups[key] = new ProcessorLinkGroup(linkId, device);
                groups[key].AddMember(instance.id, instance.processors[device]);
            }
        }
    }
    this.processorGroups = groups;
};

BankManagerLinkGraph.prototype.ProcessorGroup = function(linkId, device) {
    return this.processorGroups[String(linkId) + ":" + String(device)] || null;
};

BankManagerLinkGraph.prototype.Members = function(linkId) {
    var members = [];
    var rows = this.manager.Rows();
    for (var rowIndex = 0; rowIndex < rows.length; ++rowIndex) {
        for (var bankIndex = 0; bankIndex < rows[rowIndex].banks.length; ++bankIndex) {
            var bank = rows[rowIndex].banks[bankIndex];
            if (bank.linkId === linkId) {
                members.push({ instance: rows[rowIndex], bank: bank });
                break;
            }
        }
    }
    return members;
};

BankManagerLinkGraph.prototype.MemberIds = function(linkId) {
    return this.Members(linkId).map(function(member) {
        return member.instance.id;
    }).sort();
};

BankManagerLinkGraph.prototype.FindLocalBank = function(linkId) {
    var local = this.manager.local;
    var active = local.banks[local.selectedBankId - 1];
    if (active && active.linkId === linkId) return active;
    for (var index = 0; index < local.banks.length; ++index) {
        if (local.banks[index].linkId === linkId) return local.banks[index];
    }
    return null;
};
