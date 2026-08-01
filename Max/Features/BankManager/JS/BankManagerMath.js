function BankManagerMath() {}

BankManagerMath.Normalize = function(value, range) {
    var number = Number(value);
    if (!range || !isFinite(number) || range.maximum === range.minimum) return NaN;
    if (range.logarithmic && range.minimum > 0 && number > 0) {
        return Math.log(number / range.minimum)
            / Math.log(range.maximum / range.minimum);
    }
    return (number - range.minimum) / (range.maximum - range.minimum);
};

BankManagerMath.Denormalize = function(value, range) {
    var normalized = Math.max(0, Math.min(1, Number(value)));
    if (!range || !isFinite(normalized)) return NaN;
    if (range.logarithmic && range.minimum > 0) {
        return range.minimum
            * Math.pow(range.maximum / range.minimum, normalized);
    }
    return range.minimum + normalized * (range.maximum - range.minimum);
};
