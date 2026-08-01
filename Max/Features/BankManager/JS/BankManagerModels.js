function BankSummary() {
    this.id = 0;
    this.occupied = false;
    this.linkId = "";
    this.filters = {};
}

function ProcessorSummary(id, label) {
    this.id = id;
    this.label = label;
    this.values = {};
}

function ProcessorLinkGroup(linkId, device) {
    this.linkId = linkId;
    this.device = device;
    this.members = {};
}

ProcessorLinkGroup.prototype.AddMember = function(instanceId, processor) {
    this.members[instanceId] = processor;
};

ProcessorLinkGroup.prototype.EffectiveRange = function(sourceId, parameter, range) {
    var source = this.members[sourceId];
    var sourceValue = source
        ? BankManagerMath.Normalize(source.values[parameter], range)
        : NaN;
    if (!range || !isFinite(sourceValue) || Object.keys(this.members).length < 2) return range;
    var minimumDelta = -Infinity;
    var maximumDelta = Infinity;
    for (var instanceId in this.members) {
        if (!this.members.hasOwnProperty(instanceId) || instanceId === sourceId) continue;
        var value = BankManagerMath.Normalize(this.members[instanceId].values[parameter], range);
        if (!isFinite(value)) {
            var lockedValue = BankManagerMath.Denormalize(sourceValue, range);
            return { minimum: lockedValue, maximum: lockedValue };
        }
        minimumDelta = Math.max(minimumDelta, -value);
        maximumDelta = Math.min(maximumDelta, 1 - value);
    }
    return {
        minimum: BankManagerMath.Denormalize(Math.max(0, sourceValue + minimumDelta), range),
        maximum: BankManagerMath.Denormalize(Math.min(1, sourceValue + maximumDelta), range)
    };
};

ProcessorLinkGroup.prototype.ApplyDelta = function(
    sourceId,
    parameter,
    delta,
    sourceAlreadyApplied,
    range
) {
    for (var instanceId in this.members) {
        if (!this.members.hasOwnProperty(instanceId) ||
            (sourceAlreadyApplied && instanceId === sourceId)) continue;
        var processor = this.members[instanceId];
        var normalized = BankManagerMath.Normalize(processor.values[parameter], range);
        if (isFinite(normalized)) {
            processor.values[parameter] = BankManagerMath.Denormalize(normalized + delta, range);
        }
    }
};

function InstanceSummary(id, label) {
    this.id = id;
    this.label = label;
    this.trackOrder = Infinity;
    this.revision = 0;
    this.selectedBankId = 1;
    this.systemBank = new BankSummary();
    this.banks = [];
    for (var bankId = 1; bankId <= 6; ++bankId) {
        var bank = new BankSummary();
        bank.id = bankId;
        this.banks.push(bank);
    }
    this.processors = {
        compressor: new ProcessorSummary("compressor", "COMP"),
        saturator: new ProcessorSummary("saturator", "SAT"),
        input_gain: new ProcessorSummary("input_gain", "IN"),
        output_gain: new ProcessorSummary("output_gain", "OUT")
    };
}
