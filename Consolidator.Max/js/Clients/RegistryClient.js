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
    protocol.on("registry_changed", this.handleChanged.bind(this));
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

RegistryClient.prototype.handleChanged = function (args) {
    var revision = Number(args[1]);
    if (!isFinite(revision)) {
        return;
    }
    this.requiredRevision = Math.max(this.requiredRevision, revision);
    if (!this.fetchPending &&
        (!this.snapshot || this.snapshot.revision < this.requiredRevision)) {
        this.fetch();
    }
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
        selectedBank: args[5],
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

    if (this.snapshot && response.revision <= this.snapshot.revision) {
        this.protocol.complete(requestId, { snapshot: this.snapshot });
        return;
    }
    this.snapshot = {
        revision: response.revision,
        instances: response.instances,
        groups: response.groups
    };
    this.notify(this.snapshot);
    this.protocol.complete(requestId, { snapshot: this.snapshot });
};

RegistryClient.prototype.handleError = function (args) {
    var requestId = String(args[2]);
    delete this.responses[requestId];
};

RegistryClient.prototype.notify = function (snapshot) {
    var listeners = this.subscribers.slice();
    for (var index = 0; index < listeners.length; index += 1) {
        listeners[index](snapshot);
    }
};

RegistryClient.prototype.destroy = function () {
    this.snapshot = null;
    this.subscribers = [];
    this.responses = {};
    this.fetchPending = false;
};
