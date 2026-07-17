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
    dictionary.setparse("payload", "{}");
    for (var key in this.payload) {
        if (this.payload.hasOwnProperty(key)) {
            dictionary.replace("payload::" + key, this.payload[key]);
        }
    }
    return dictionary;
};

MessageEnvelope.fromMaxDictionary = function(value) {
    try {
        var dictionary;
        if (value && typeof value === "object" && value.name) {
            dictionary = value;
        }
        else {
            var name = MessageEnvelope.dictionaryName(value);
            if (!name) return null;
            dictionary = new Dict(name);
        }
        var type = dictionary.get("type");
        if (!type) return null;
        var envelope = MessageEnvelope.fromObject({
            type: String(type),
            target: dictionary.get("target"),
            source: dictionary.get("source"),
            payload: {}
        });
        envelope.dictionary = dictionary;
        return envelope;
    }
    catch (error) {
        return null;
    }
};

MessageEnvelope.dictionaryName = function(value) {
    if (value instanceof Array) {
        var text = value.map(function(item) { return String(item); }).join(" ");
        var match = /^dictionary\s+([^\s]+)/.exec(text);
        if (match) {
            return match[1];
        }
        if (value.length === 1) return MessageEnvelope.dictionaryName(value[0]);
        return "";
    }
    var text = String(value);
    var match = /^dictionary\s+([^\s]+)/.exec(text);
    return match ? match[1] : text;
};

MessageEnvelope.prototype.payloadValue = function(key) {
    if (this.dictionary) {
        return this.dictionary.get("payload::" + key);
    }
    return this.payload[key];
};
