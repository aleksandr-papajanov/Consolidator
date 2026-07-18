function DictionaryReader(reference) {
    this.dict = DictionaryReader.Open(reference);
    this.name = this.dict.name;
    this.object = JSON.parse(this.dict.stringify());

    for (var key in this.object) {
        if (Object.prototype.hasOwnProperty.call(this.object, key)) {
            this[key] = this.object[key];
        }
    }
}

DictionaryReader.Open = function(reference) {
    if (reference && typeof reference === "object" && reference.name) {
        return reference;
    }

    var name = DictionaryReader.Name(reference);
    if (!name) throw new Error("missing_dictionary_name");
    return new Dict(name);
};

DictionaryReader.Name = function(reference) {
    if (reference instanceof Array) {
        if (reference.length === 1) return DictionaryReader.Name(reference[0]);
        var text = reference.map(function(value) {
            return String(value);
        }).join(" ");
        var listMatch = /^dictionary\s+([^\s]+)/.exec(text);
        return listMatch ? listMatch[1] : "";
    }

    var text = String(reference || "");
    var match = /^dictionary\s+([^\s]+)/.exec(text);
    return match ? match[1] : text;
};
