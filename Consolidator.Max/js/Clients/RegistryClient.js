function RegistryClient(protocol) {
    this.protocol = protocol;
    this.snapshot = null;
    this.subscribers = [];
    this.responses = {};
    this.fetchPending = false;
    this.requiredRevision = 0;

    protocol.on("registry_begin", this.handleBegin.bind(this));
    protocol.on("registry_instance", this.handleInstance.bind(this));
    protocol.on("registry_bank", this.handleBank.bind(this));
    protocol.on("registry_group", this.handleGroup.bind(this));
    protocol.on("registry_member", this.handleMember.bind(this));
    protocol.on("registry_done", this.handleDone.bind(this));
    protocol.on("registry_instance_added", this.handleDelta.bind(this, "registry_instance_added"));
    protocol.on("registry_instance_removed", this.handleDelta.bind(this, "registry_instance_removed"));
    protocol.on("registry_label_changed", this.handleDelta.bind(this, "registry_label_changed"));
    protocol.on("registry_bank_group_changed", this.handleDelta.bind(this, "registry_bank_group_changed"));
    protocol.on("error", this.handleError.bind(this));
}

RegistryClient.prototype.get = function () {
    return this.snapshot;
};

RegistryClient.prototype.fetch = function (callback) {
    var self = this;
    this.fetchPending = true;
    return this.protocol.request("registry", [], function (response) {
        self.fetchPending = false;
        if (callback) callback(response.error ? undefined : self.snapshot, response);
        if (!response.error &&
            (!self.snapshot ||
             self.snapshot.revision < self.requiredRevision)) {
            self.fetch();
        }
    });
};

RegistryClient.prototype.handleDelta = function (selector, args) {
    var previousRevision = Number(args[1]);
    var revision = Number(args[2]);
    if (!isFinite(previousRevision) || !isFinite(revision)) {
        return;
    }
    if (!this.snapshot || this.snapshot.revision !== previousRevision) {
        this.requiredRevision = Math.max(this.requiredRevision, revision);
        if (!this.fetchPending) this.fetch();
        return;
    }

    if (selector === "registry_instance_added") {
        this.applyInstanceAdded(args);
    } else if (selector === "registry_instance_removed") {
        this.applyInstanceRemoved(args);
    } else if (selector === "registry_label_changed") {
        this.applyLabelChanged(args);
    } else if (selector === "registry_bank_group_changed") {
        this.applyBankGroupChanged(args);
    } else {
        return;
    }
    this.snapshot.revision = revision;
    this.notify(this.snapshot, { selector: selector, args: args });
};

RegistryClient.prototype.applyInstanceAdded = function (args) {
    var instanceId = args[3];
    var instance = { instanceId: instanceId, label: String(args[4]), banks: [] };
    var count = Number(args[5]);
    for (var index = 0; index < count; index += 1) {
        var position = 6 + index * 2;
        instance.banks.push({
            bankId: args[position],
            groupId: args[position + 1] === "none" ? null : args[position + 1]
        });
    }
    this.snapshot.instances.push(instance);
    this.updateGroupsForInstance(instanceId, this.snapshot.instances);
};

RegistryClient.prototype.applyInstanceRemoved = function (args) {
    var instanceId = String(args[3]);
    this.snapshot.instances = this.snapshot.instances.filter(function (instance) {
        return String(instance.instanceId) !== instanceId;
    });
    this.updateGroupsForInstance(instanceId, this.snapshot.instances);
};

RegistryClient.prototype.applyLabelChanged = function (args) {
    var instanceId = String(args[3]);
    this.snapshot.instances.forEach(function (instance) {
        if (String(instance.instanceId) === instanceId) instance.label = String(args[4]);
    });
};

RegistryClient.prototype.applyBankGroupChanged = function (args) {
    var instanceId = String(args[3]);
    var bankId = Number(args[4]);
    var groupId = args[5] === "none" ? null : args[5];
    this.snapshot.instances.forEach(function (instance) {
        if (String(instance.instanceId) !== instanceId) return;
        instance.banks.forEach(function (bank) {
            if (Number(bank.bankId) === bankId) bank.groupId = groupId;
        });
    });
    this.updateGroupsForInstance(instanceId, this.snapshot.instances);
};

RegistryClient.prototype.updateGroupsForInstance = function (instanceId, instances) {
    var groups = this.snapshot.groups || [];
    groups.forEach(function (group) {
        group.members = group.members.filter(function (member) {
            return String(member.instanceId) !== String(instanceId);
        });
    });
    var instance = instances.filter(function (candidate) {
        return String(candidate.instanceId) === String(instanceId);
    })[0];
    if (instance) {
        instance.banks.forEach(function (bank) {
            if (bank.groupId === null || bank.groupId === undefined) return;
            var group = groups.filter(function (candidate) {
                return String(candidate.groupId) === String(bank.groupId);
            })[0];
            if (!group) {
                group = { groupId: bank.groupId, members: [] };
                groups.push(group);
            }
            group.members.push({ instanceId: instance.instanceId, bankId: bank.bankId });
        });
    }
    this.snapshot.groups = groups.filter(function (group) {
        return group.members.length > 0;
    });
};

RegistryClient.prototype.subscribe = function (callback, immediate) {
    this.subscribers.push(callback);
    if (immediate && this.snapshot) callback(this.snapshot);
    var self = this;
    return function () {
        self.subscribers = self.subscribers.filter(function (listener) {
            return listener !== callback;
        });
    };
};

RegistryClient.prototype.handleBegin = function (args) {
    var requestId = String(args[2]);
    this.responses[requestId] = {
        revision: Number(args[3]),
        instances: [],
        groups: [],
        instancesById: {},
        groupsById: {}
    };
};

RegistryClient.prototype.handleInstance = function (args) {
    var response = this.responses[String(args[2])];
    if (!response) return;
    var instance = {
        instanceId: args[3],
        label: String(args[4]),
        banks: []
    };
    response.instances.push(instance);
    response.instancesById[String(instance.instanceId)] = instance;
};

RegistryClient.prototype.handleBank = function (args) {
    var response = this.responses[String(args[2])];
    if (!response) return;
    var instance = response.instancesById[String(args[3])];
    if (!instance) return;
    instance.banks.push({
        bankId: args[4],
        groupId: args[5] === "none" ? null : args[5]
    });
};

RegistryClient.prototype.handleGroup = function (args) {
    var response = this.responses[String(args[2])];
    if (!response) return;
    var group = { groupId: args[3], members: [] };
    response.groups.push(group);
    response.groupsById[String(group.groupId)] = group;
};

RegistryClient.prototype.handleMember = function (args) {
    var response = this.responses[String(args[2])];
    if (!response) return;
    var group = response.groupsById[String(args[3])];
    if (!group) return;
    group.members.push({ instanceId: args[4], bankId: args[5] });
};

RegistryClient.prototype.handleDone = function (args) {
    var requestId = String(args[2]);
    var response = this.responses[requestId];
    if (!response) return;
    delete this.responses[requestId];

    if (response.revision < this.requiredRevision) {
        this.protocol.complete(requestId, { snapshot: this.snapshot });
        return;
    }

    if (this.snapshot && response.revision <= this.snapshot.revision) {
        this.protocol.complete(requestId, { snapshot: this.snapshot });
        return;
    }
    this.snapshot = {
        revision: response.revision,
        instances: response.instances,
        groups: response.groups
    };
    this.requiredRevision = 0;
    this.notify(this.snapshot);
    this.protocol.complete(requestId, { snapshot: this.snapshot });
};

RegistryClient.prototype.handleError = function (args) {
    var requestId = String(args[2]);
    delete this.responses[requestId];
};

RegistryClient.prototype.notify = function (snapshot, delta) {
    var listeners = this.subscribers.slice();
    for (var index = 0; index < listeners.length; index += 1) {
        listeners[index](snapshot, delta);
    }
};

RegistryClient.prototype.destroy = function () {
    this.snapshot = null;
    this.subscribers = [];
    this.responses = {};
    this.fetchPending = false;
    this.requiredRevision = 0;
};
