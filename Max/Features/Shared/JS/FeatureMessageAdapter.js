function FeatureMessageAdapter(featureId, messageFactory, handlers) {
    this.featureId = String(featureId);
    this.messageFactory = messageFactory;
    this.handlers = handlers || {};
}

FeatureMessageAdapter.prototype.AcceptBusEnvelope = function(dictionaryName) {
    var message = this.messageFactory.fromMax(dictionaryName);
    if (!message || !this.AcceptsTarget(message.target)) {
        return false;
    }

    if (this.handlers.OnEnvelope) {
        this.handlers.OnEnvelope(message);
    }
    return true;
};

FeatureMessageAdapter.prototype.AcceptsTarget = function(target) {
    return target === "broadcast" || String(target) === this.featureId;
};

FeatureMessageAdapter.prototype.EmitEnvelope = function(message, emit) {
    if (!message || String(message.source) !== this.featureId) {
        return false;
    }
    emit(message);
    return true;
};

FeatureMessageAdapter.prototype.HandleStatus = function(state, details) {
    if (this.handlers.OnStatus) {
        this.handlers.OnStatus(String(state), details || []);
    }
};
