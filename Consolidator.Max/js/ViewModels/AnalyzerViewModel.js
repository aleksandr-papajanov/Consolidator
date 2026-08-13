function AnalyzerViewModel(analysis) {
    this.analysis = analysis;
    this.mainSpectrum = new ObservableValue();
    this.referenceSpectrum = new ObservableValue();
    this.differenceSpectrum = new ObservableValue();
    this.combinedCurve = new ObservableValue();
    this.allBanksCurve = new ObservableValue();
    this.filterCurves = [];
    this.compressorDetectorCurves = [];
    this.saturatorDetectorCurves = [];
    this.compressorDetectorCombined = new ObservableValue();
    this.saturatorDetectorCombined = new ObservableValue();
    this.inputGainMeter = new ObservableValue();
    this.saturatorMeter = new ObservableValue();
    this.compressorMeter = new ObservableValue();
    this.outputGainMeter = new ObservableValue();
    this.compressorReduction = new ObservableValue();
    this.saturatorDistortion = new ObservableValue();
    this.unsubscribers = [];

    this.subscribeAnalysis("spectrum.main", this.mainSpectrum);
    this.subscribeAnalysis("spectrum.reference", this.referenceSpectrum);
    this.subscribeAnalysis("spectrum.difference", this.differenceSpectrum);
    this.subscribeAnalysis("eq.combined", this.combinedCurve);
    this.subscribeAnalysis("eq.allBanks", this.allBanksCurve);

    for (var filterId = 1; filterId <= 7; filterId += 1) {
        var filterCurve = new ObservableValue();
        this.filterCurves.push(filterCurve);
        this.subscribeAnalysis("eq.filter." + filterId, filterCurve);
    }
    this.subscribeDetector("compressor", this.compressorDetectorCurves,
        this.compressorDetectorCombined);
    this.subscribeDetector("saturator", this.saturatorDetectorCurves,
        this.saturatorDetectorCombined);

    this.subscribeAnalysis("meter.input_gain", this.inputGainMeter);
    this.subscribeAnalysis("meter.saturator", this.saturatorMeter);
    this.subscribeAnalysis("meter.compressor", this.compressorMeter);
    this.subscribeAnalysis("meter.output_gain", this.outputGainMeter);
    this.subscribeAnalysis("compressor.reduction", this.compressorReduction);
    this.subscribeAnalysis("saturator.distortion", this.saturatorDistortion);
}

AnalyzerViewModel.prototype.subscribeAnalysis = function (key, target) {
    this.unsubscribers.push(this.analysis.subscribe(key, function (value) {
        target.set(value);
    }, true));
};

AnalyzerViewModel.prototype.subscribeDetector = function (
    device, curves, combined
) {
    var self = this;
    for (var filterId = 1; filterId <= 2; filterId += 1) {
        var curve = new ObservableValue();
        curves.push(curve);
        self.subscribeAnalysis("detector." + device + ".filter." + filterId,
            curve);
    }
    self.subscribeAnalysis("detector." + device + ".combined", combined);
};

AnalyzerViewModel.prototype.show = function (instanceId, bankId) {
    this.analysis.view(instanceId, bankId);
};

AnalyzerViewModel.prototype.destroy = function () {
    for (var index = 0; index < this.unsubscribers.length; index += 1) {
        this.unsubscribers[index]();
    }
    this.unsubscribers = [];
};
