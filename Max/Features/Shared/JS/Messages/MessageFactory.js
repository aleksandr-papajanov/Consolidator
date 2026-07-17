function MessageFactory() {
}

MessageFactory.types = {};

MessageFactory.register = function(type, constructor) {
    MessageFactory.types[String(type)] = constructor;
};

MessageFactory.create = function(type, target, payload, source) {
    var constructor = MessageFactory.types[String(type)];
    return constructor
        ? constructor.create(target, payload, source)
        : MessageEnvelope.create(type, target, payload, source);
};

MessageFactory.fromObject = function(value) {
    var envelope = MessageEnvelope.fromObject(value);
    if (!envelope) return null;
    var constructor = MessageFactory.types[envelope.type];
    return constructor && constructor.fromEnvelope
        ? constructor.fromEnvelope(envelope)
        : envelope;
};

MessageFactory.fromJson = function(value) {
    try {
        return MessageFactory.fromObject(JSON.parse(String(value)));
    }
    catch (error) {
        return null;
    }
};

MessageFactory.toJson = function(message) {
    return message && message.toJson ? message.toJson() : "";
};

MessageFactory.fromMax = function(value) {
    var envelope = MessageEnvelope.fromMaxDictionary(value);
    return envelope;
};

MessageFactory.toMax = function(message) {
    return message && message.toMaxDictionary ? message.toMaxDictionary() : null;
};
