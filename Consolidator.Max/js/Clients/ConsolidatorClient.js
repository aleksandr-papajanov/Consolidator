function ConsolidatorClient(source, send) {
    this.protocol = new NativeProtocolClient(source, send);
    this.state = new StateClient(this.protocol);
    this.transactions = new TransactionClient(this.protocol);
    this.targetState = new TargetStateClient(this.protocol, this.state);
    this.uiTarget = new UiTarget(this.targetState);
    this.registry = new RegistryClient(this.protocol);
}

ConsolidatorClient.prototype.initialize = function (callback) {
    return this.protocol.initialize(callback);
};

ConsolidatorClient.prototype.setInstanceActive = function (active, callback) {
    return this.protocol.request(
        "set_instance_active",
        [active ? 1 : 0],
        callback);
};

ConsolidatorClient.prototype.handleControl = function (selector, args) {
    this.protocol.handleControl(selector, args);
};

ConsolidatorClient.prototype.destroy = function () {
    this.state.destroy();
    this.transactions.destroy();
    this.targetState.destroy();
    this.uiTarget.destroy();
    this.registry.destroy();
    this.protocol.destroy();
};
