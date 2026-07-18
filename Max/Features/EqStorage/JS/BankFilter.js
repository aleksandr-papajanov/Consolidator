function BankFilter(id, values, bypass) {
    this.id = String(id);
    this.values = values ? toFilterValues(values) : [];
    this.bypass = bypass === undefined ? 0 : Number(bypass);
}

BankFilter.prototype.Update = function(values, bypass) {
    this.values = toFilterValues(values);
    this.bypass = Number(bypass);
};

BankFilter.prototype.IsDefined = function() {
    return this.values.length > 0;
};

function toFilterValues(value) {
    if (value === null || value === undefined || value === "") {
        return [];
    }
    return value instanceof Array ? value : [value];
}
