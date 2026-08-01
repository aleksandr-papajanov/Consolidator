function DetectorFilterViewModel() {
    this.bypass = null;
    this.gainDb = null;
    this.frequencyHz = null;
    this.q = null;
    this.definition = null;
    this.limits = {};
    this.hasState = false;
}

DetectorFilterViewModel.prototype.SetLimit = function(parameter, minimum, maximum) {
    this.limits[String(parameter)] = {
        minimum: Number(minimum),
        maximum: Number(maximum)
    };
};

DetectorFilterViewModel.prototype.Limit = function(parameter, minimum, maximum) {
    var limit = this.limits[String(parameter)];
    return limit || { minimum: Number(minimum), maximum: Number(maximum) };
};

DetectorFilterViewModel.prototype.Update = function(bypass, gainDb, frequencyHz, q) {
    this.bypass = Number(bypass) !== 0;
    this.gainDb = Number(gainDb);
    this.frequencyHz = Number(frequencyHz);
    this.q = Number(q);
    this.hasState = true;
};

DetectorFilterViewModel.prototype.SetDefinition = function(definition) {
    this.definition = definition;
    if (this.hasState) return;
    this.bypass = definition.bypassDefault;
    this.gainDb = definition.gainDefault;
    this.frequencyHz = definition.frequencyDefault;
    this.q = definition.qDefault;
};

DetectorFilterViewModel.prototype.Reset = function() {
    if (!this.definition) return;
    this.Update(
        this.definition.bypassDefault,
        this.definition.gainDefault,
        this.definition.frequencyDefault,
        this.definition.qDefault
    );
};

function DetectorCurveViewModel() {
    this.filters = [];
    this.listenFilters = {};
    this.linkColor = null;
    this.EnsureFilter(1);
    this.EnsureFilter(2);
}

DetectorCurveViewModel.prototype.SetLinkColor = function(
    linkId,
    red,
    green,
    blue,
    alpha
) {
    if (String(linkId) === "-") {
        this.linkColor = null;
        return;
    }
    this.linkColor = [
        Number(red), Number(green), Number(blue), Number(alpha)
    ];
};

DetectorCurveViewModel.prototype.SetLimit = function(filterId, parameter, minimum, maximum) {
    var filter = this.EnsureFilter(filterId);
    if (!filter) return;
    filter.SetLimit(parameter, minimum, maximum);
};

DetectorCurveViewModel.prototype.SetPreview = function(filterId, parameter, value) {
    var filter = this.EnsureFilter(filterId);
    if (!filter) return;
    if (parameter === "bypass") filter.bypass = Number(value) !== 0;
    else if (parameter === "gain") filter.gainDb = Number(value);
    else if (parameter === "frequency") filter.frequencyHz = Number(value);
    else if (parameter === "q") filter.q = Number(value);
};

DetectorCurveViewModel.prototype.Reset = function(filterId) {
    var filter = this.EnsureFilter(filterId);
    if (!filter) return;
    filter.Reset();
    this.listenFilters[Number(filterId)] = false;
};

DetectorCurveViewModel.prototype.EnsureFilter = function(filterId) {
    var index = Number(filterId) - 1;
    if (index < 0) return null;
    while (this.filters.length <= index) {
        this.filters.push(new DetectorFilterViewModel());
    }
    return this.filters[index];
};

DetectorCurveViewModel.prototype.SetDetector = function(
    filterId,
    bypass,
    gainDb,
    frequencyHz,
    q
) {
    var filter = this.EnsureFilter(filterId);
    if (!filter) return false;
    filter.Update(bypass, gainDb, frequencyHz, q);
    if (filter.bypass) this.listenFilters[Number(filterId)] = false;
    return true;
};

DetectorCurveViewModel.prototype.SetDefinition = function(
    filterId,
    gainMinimum,
    gainMaximum,
    gainDefault,
    frequencyMinimum,
    frequencyMaximum,
    frequencyDefault,
    qMinimum,
    qMaximum,
    qDefault,
    bypassDefault
) {
    var filter = this.EnsureFilter(filterId);
    if (!filter) return false;
    filter.SetDefinition({
        gainMinimum: Number(gainMinimum),
        gainMaximum: Number(gainMaximum),
        gainDefault: Number(gainDefault),
        frequencyMinimum: Number(frequencyMinimum),
        frequencyMaximum: Number(frequencyMaximum),
        frequencyDefault: Number(frequencyDefault),
        qMinimum: Number(qMinimum),
        qMaximum: Number(qMaximum),
        qDefault: Number(qDefault),
        bypassDefault: Number(bypassDefault) !== 0
    });
    return true;
};

DetectorCurveViewModel.prototype.SetCatalogDefinitions = function(definitions) {
    for (var id in definitions) {
        if (!definitions.hasOwnProperty(id)) continue;
        var definition = definitions[id];
        var gain = this.FindParameter(definition, "gain");
        var frequency = this.FindParameter(definition, "frequency");
        var q = this.FindParameter(definition, "q");
        if (!gain || !frequency || !q) continue;
        this.SetDefinition(
            definition.id,
            gain.minimum,
            gain.maximum,
            gain.defaultValue,
            frequency.minimum,
            frequency.maximum,
            frequency.defaultValue,
            q.minimum,
            q.maximum,
            q.defaultValue,
            definition.defaultBypass ? 1 : 0
        );
    }
};

DetectorCurveViewModel.prototype.FindParameter = function(definition, name) {
    for (var index = 0; index < definition.parameters.length; index++) {
        if (definition.parameters[index].name === name) {
            return definition.parameters[index];
        }
    }
    return null;
};

DetectorCurveViewModel.prototype.SetListen = function(filterId, enabled) {
    var id = Math.max(1, Number(filterId));
    var filter = this.EnsureFilter(id);
    if (!filter || filter.bypass) {
        this.listenFilters[id] = false;
        return;
    }
    this.listenFilters[id] = Number(enabled) !== 0;
};

DetectorCurveViewModel.prototype.IsListening = function(filterId) {
    return this.listenFilters[Number(filterId)] === true;
};

DetectorCurveViewModel.prototype.BuildCurve = function(filterIndex) {
    var options = detectorCurveOptions;
    var filter = this.filters[filterIndex];
    var values = [];
    if (!filter || !filter.definition) return values;
    for (var point = 0; point < options.pointCount; point++) {
        var normalized = point / (options.pointCount - 1);
        var frequencyHz = options.minimumFrequencyHz * Math.pow(
            options.maximumFrequencyHz / options.minimumFrequencyHz,
            normalized
        );
        values.push(this.MagnitudeDb(filter, frequencyHz));
    }
    return values;
};

DetectorCurveViewModel.prototype.BuildTotalCurve = function() {
    if (!this.filters.length) return [];
    var total = this.BuildCurve(0);
    for (var filterIndex = 1; filterIndex < this.filters.length; filterIndex++) {
        var curve = this.BuildCurve(filterIndex);
        for (var index = 0; index < total.length; index++) {
            total[index] += curve[index];
        }
    }
    return total;
};

DetectorCurveViewModel.prototype.MagnitudeDb = function(filter, frequencyHz) {
    var options = detectorCurveOptions;
    if (!filter || !filter.definition || filter.bypass ||
        filter.gainDb === null || filter.frequencyHz === null || filter.q === null ||
        Math.abs(filter.gainDb) < 0.000001) return 0.0;

    var amplitude = Math.pow(10.0, filter.gainDb / 40.0);
    var omega = 2.0 * Math.PI * filter.frequencyHz / options.sampleRate;
    var alpha = Math.sin(omega) / (2.0 * Math.max(0.01, filter.q));
    var cosine = Math.cos(omega);
    var b0 = 1.0 + alpha * amplitude;
    var b1 = -2.0 * cosine;
    var b2 = 1.0 - alpha * amplitude;
    var a0 = 1.0 + alpha / amplitude;
    var a1 = -2.0 * cosine;
    var a2 = 1.0 - alpha / amplitude;
    var evaluationOmega = 2.0 * Math.PI * frequencyHz / options.sampleRate;
    var evaluationCosine = Math.cos(evaluationOmega);
    var evaluationSine = Math.sin(evaluationOmega);
    var doubleCosine = Math.cos(evaluationOmega * 2.0);
    var doubleSine = Math.sin(evaluationOmega * 2.0);
    var numeratorReal = b0 + b1 * evaluationCosine + b2 * doubleCosine;
    var numeratorImaginary = -b1 * evaluationSine - b2 * doubleSine;
    var denominatorReal = a0 + a1 * evaluationCosine + a2 * doubleCosine;
    var denominatorImaginary = -a1 * evaluationSine - a2 * doubleSine;
    var numeratorPower = numeratorReal * numeratorReal + numeratorImaginary * numeratorImaginary;
    var denominatorPower = denominatorReal * denominatorReal + denominatorImaginary * denominatorImaginary;
    return 10.0 * Math.log(Math.max(1.0e-12, numeratorPower / denominatorPower)) / Math.LN10;
};
