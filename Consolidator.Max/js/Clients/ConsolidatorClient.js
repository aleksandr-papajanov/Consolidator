function ConsolidatorClient(source, send) {
    this.protocol = new NativeProtocolClient(source, send);
    this.state = new StateClient(this.protocol);
    this.analysis = new AnalysisClient(this.protocol);
    this.registry = new RegistryClient(this.protocol);
}

ConsolidatorClient.prototype.handleControl = function (selector, args) {
    this.protocol.handleControl(selector, args);
};

ConsolidatorClient.prototype.handleAnalysis = function (selector, args) {
    this.protocol.handleAnalysis(selector, args);
};

ConsolidatorClient.prototype.destroy = function () {
    this.state.destroy();
    this.analysis.destroy();
    this.registry.destroy();
    this.protocol.destroy();
};
