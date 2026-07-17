function BankFilter(id, values, bypass) {
    this.id = String(id);
    this.values = values ? normalizeFilterValues(values) : [];
    this.bypass = bypass === undefined ? 0 : Number(bypass);
}

BankFilter.prototype.update = function(values, bypass) {
    this.values = normalizeFilterValues(values);
    this.bypass = Number(bypass);
};

BankFilter.prototype.isDefined = function() {
    return this.values.length > 0;
};

BankFilter.prototype.valueMessage = function() {
    return MessageEnvelope.create("filter.update", "filter", {
        filterId: Number(this.id),
        values: this.values
    }, "eq.storage");
};

BankFilter.prototype.bypassMessage = function() {
    return MessageEnvelope.create("filter.bypass", "filter", {
        filterId: Number(this.id),
        value: this.bypass
    }, "eq.storage");
};

function normalizeFilterValues(value) {
    if (value === null || value === undefined || value === "") {
        return [];
    }
    return value instanceof Array ? value : [value];
}
