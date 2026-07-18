function MessageEnvelope(type, target, source, payload) {
    this.type = String(type);
    this.target = target === undefined || target === null ? "broadcast" : String(target);
    this.source = source === undefined || source === null ? "" : String(source);
    this.payload = payload || {};
}

MessageEnvelope.create = function(type, target, payload, source) {
    return new MessageEnvelope(type, target, source, payload);
};
MessageEnvelope.fromObject = function(value) {
    if (!value || typeof value.type !== "string") {
        return null;
    }
    return new MessageEnvelope(value.type, value.target, value.source, value.payload);
};
MessageEnvelope.prototype.toObject = function() {
    var result = {
        type: this.type,
        payload: this.payload
    };
    result.target = this.target;
    result.source = this.source;
    return result;
};

MessageEnvelope.prototype.toJson = function() {
    return JSON.stringify(this.toObject());
};

MessageEnvelope.prototype.toMaxDictionary = function() {
    var dictionary = new Dict();
    dictionary.set("type", this.type);
    dictionary.set("target", this.target);
    dictionary.set("source", this.source);
    dictionary.setparse("payload", JSON.stringify(this.payload));
    return dictionary;
};

MessageEnvelope.fromMaxDictionary = function(value) {
    try {
        var reader = new DictionaryReader(value);
        var envelope = MessageEnvelope.fromObject(reader.object);
        if (envelope) envelope.dict = reader.name;
        return envelope;
    }
    catch (error) {
        return null;
    }
};
