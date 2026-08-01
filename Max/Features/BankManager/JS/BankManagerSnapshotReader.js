function BankManagerSnapshotReader(filterDefinitions) {
    this.filterDefinitions = filterDefinitions;
}

BankManagerSnapshotReader.prototype.ReadEq = function(values) {
    if (values.length < 10 || String(values[0]) !== "snapshot" ||
        Number(values[1]) !== 1 || String(values[2]) !== "host" ||
        String(values[3]) !== "eq") return null;
    var revision = Number(values[4]);
    var selectedBankId = Number(values[5]);
    var bankCount = Number(values[8]);
    if (!isFinite(revision) || selectedBankId < 1 || selectedBankId > 6 ||
        bankCount !== 7) return null;
    var position = 9;
    var banks = [];
    var systemBank = new BankSummary();
    for (var index = 0; index < bankCount; ++index) {
        if (position + 2 >= values.length) return null;
        var bankId = Number(values[position++]);
        var linkId = String(values[position++]);
        var filterCount = Number(values[position++]);
        if (bankId !== index || !isFinite(filterCount) || filterCount < 0) return null;
        var occupied = false;
        var filters = {};
        for (var filterIndex = 0; filterIndex < filterCount; ++filterIndex) {
            if (position + 2 >= values.length) return null;
            var filterId = Number(values[position++]);
            var bypass = Number(values[position++]) !== 0;
            var valueCount = Number(values[position++]);
            if (!isFinite(valueCount) || valueCount < 0 ||
                position + valueCount > values.length) return null;
            var filterValues = [];
            var parameters = this.filterDefinitions[filterId] || [];
            for (var valueIndex = 0; valueIndex < valueCount; ++valueIndex) {
                var value = Number(values[position + valueIndex]);
                filterValues.push(value);
                if (parameters[valueIndex] && parameters[valueIndex].name === "gain" &&
                    Math.abs(value) > 1.0e-12) occupied = true;
            }
            position += valueCount;
            filters[filterId] = { bypass: bypass, values: filterValues };
        }
        if (bankId === 0) {
            systemBank.occupied = occupied;
            systemBank.filters = filters;
        } else {
            var bank = new BankSummary();
            bank.id = bankId;
            bank.occupied = occupied;
            bank.linkId = linkId;
            bank.filters = filters;
            banks.push(bank);
        }
    }
    if (position !== values.length) return null;
    return {
        revision: revision,
        selectedBankId: selectedBankId,
        bypass: Number(values[6]) !== 0,
        systemBank: systemBank,
        banks: banks
    };
};

BankManagerSnapshotReader.prototype.ReadProcessor = function(values) {
    if (values.length < 29 || String(values[3]) !== "processor") return null;
    var base = values.length - 29;
    var processors = {
        compressor: new ProcessorSummary("compressor", "COMP"),
        saturator: new ProcessorSummary("saturator", "SAT"),
        input_gain: new ProcessorSummary("input_gain", "IN"),
        output_gain: new ProcessorSummary("output_gain", "OUT")
    };
    processors.input_gain.values.gain = Number(values[base]);
    processors.compressor.values = {
        attack: Number(values[base + 2]), release: Number(values[base + 3]),
        threshold: Number(values[base + 4]), output: Number(values[base + 5]),
        mix: Number(values[base + 6])
    };
    processors.saturator.values = {
        saturation: Number(values[base + 17]), output: Number(values[base + 18])
    };
    this.ReadDetectorFilters(processors.compressor.values, values, base + 7);
    this.ReadDetectorFilters(processors.saturator.values, values, base + 19);
    processors.output_gain.values.gain = Number(values[base + 28]);
    return processors;
};

BankManagerSnapshotReader.prototype.ReadDetectorFilters = function(target, values, position) {
    for (var filterId = 1; filterId <= 2; ++filterId) {
        var base = position + (filterId - 1) * 4;
        var prefix = "detector." + filterId + ".";
        target[prefix + "bypass"] = Number(values[base]);
        target[prefix + "gain"] = Number(values[base + 1]);
        target[prefix + "frequency"] = Number(values[base + 2]);
        target[prefix + "q"] = Number(values[base + 3]);
    }
};
