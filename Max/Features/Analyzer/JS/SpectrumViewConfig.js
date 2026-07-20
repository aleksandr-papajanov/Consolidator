include("../../Shared/JS/DictionaryReader.js");

function SpectrumViewState() {
this.curves = [[], [], [], [], []];
this.filterCurves = {};
this.selectedBankId = 1;
this.requestId = 0;
this.filterColors = {};
this.handles = [];
this.draggedHandle = null;
this.draggedHandleSlot = null;
this.selectionCandidates = [];
this.selectionIndex = -1;
this.selectionX = 0;
this.selectionY = 0;
this.selectedHandleSlot = null;
this.clickWasRepeat = false;
this.clickMoved = false;
this.draggingWithAlt = false;
this.dragStartX = 0;
this.dragStartY = 0;
this.dragStartFrequency = 0;
this.dragStartGain = 0;
this.dragStartQNormalized = 0;
this.qSensitivity = 1.5;

this.minDb = -15;
this.maxDb = 15;
this.dbRangePresets = [
    { min: -15, max: 15, label: "15 dB" },
    { min: -30, max: 30, label: "30 dB" }
];
this.dbRangeIndex = 1;
this.displayMinFrequency = 10;
this.displayMaxFrequency = 20000;
this.curveMinFrequency = this.displayMinFrequency;
this.curveMaxFrequency = this.displayMaxFrequency;
this.curvePointCount = 0;

this.frequencyBands = [
    { min: 10, max: 100, width: 0.31 },
    { min: 100, max: 1000, width: 0.31 },
    { min: 1000, max: 10000, width: 0.31 },
    { min: 10000, max: 20000, width: 0.07 }
];

this.frequencyLabels = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000];
this.majorFrequencies = [100, 1000, 10000];
this.minorFrequencies = [
    10, 20, 30, 40, 50, 60, 70, 80, 90,
    200, 300, 400, 500, 600, 700, 800, 900,
    2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000
];

this.visualSettings = {
    background: { r: 0.07, g: 0.07, b: 0.07, a: 1.0 },
    reference: { r: 1.00, g: 1.00, b: 1.00, a: 0.60 },
    target: { r: 1.00, g: 0.88, b: 0.25, a: 0.60 },
    difference: { r: 1.00, g: 0.35, b: 0.35, a: 0.95 },
    filterLineWidth: 1.5,
    filterLineAlpha: 0.3,
    selectedFilterLineWidth: 3,
    selectedFilterLineAlpha: 0.9,
    filterColorTransitionDb: 6.0,
    filterColorSensitivity: 1.0,
    totalLineWidth: 2.5,
    totalColorTransitionDb: 6.0,
    totalColorNetSensitivity: 3.0,
    totalEqLineWidth: 1.0,
    totalEqLineColor: { r: 0.7, g: 0.7, b: 0.7, a: 1.0 },
    totalBaseColor: { r: 0.1, g: 0.1, b: 0.1 },
    handleRadius: 6,
    handleInnerRadius: 1,
    handleSelectedRadius: 8,
    handleActiveRadius: 10,
    handleHitRadius: 22,
    handleCycleDistance: 10,
    handleRingWidth: 1.5,
    handleSelectedRing: { r: 1.0, g: 1.0, b: 1.0, a: 0.8 },
    handleActiveRing: { r: 1.0, g: 1.0, b: 1.0, a: 1.0 },
    handleFallbackColor: { r: 1.0, g: 0.55, b: 0.1, a: 1.0 },
    handleInnerShade: 0.18,
    minorGrid: { r: 0.18, g: 0.18, b: 0.18, a: 1.0, width: 0.8 },
    majorGrid: { r: 0.28, g: 0.28, b: 0.28, a: 1.0, width: 1.8 },
    horizontalGrid: { r: 0.22, g: 0.22, b: 0.22, a: 1.0, width: 1.0 },
    zeroLine: { r: 0.45, g: 0.45, b: 0.45, a: 1.0, width: 1.5 },
    label: { r: 0.72, g: 0.72, b: 0.72, a: 1.0, size: 9 },
    frequencyLabel: { r: 0.80, g: 0.80, b: 0.80, a: 1.0, size: 9 },
    rangeLabel: { r: 0.65, g: 0.65, b: 0.65, a: 1.0, size: 9 }
};

// 0 = no temporal smoothing, 0.9 = very slow/smooth
this.smoothing = 0.75;

this.styles = [
        this.CreateCurveStyle(
        this.visualSettings.reference,
        { r: 1.00, g: 1.00, b: 1.00, a: 0.1 },
        null
    ),
    this.CreateCurveStyle(
        this.visualSettings.target,
        { r: 1.00, g: 0.88, b: 0.25, a: 0.1 },
        null
    ),
    this.CreateCurveStyle(
        { r: 1.00, g: 1.00, b: 1.00, a: 0.00 },
        { r: 1.00, g: 1.00, b: 1.00, a: 0.00 },
        { r: this.visualSettings.difference.r, g: this.visualSettings.difference.g, b: this.visualSettings.difference.b,
            a: this.visualSettings.difference.a, width: this.visualSettings.totalLineWidth }
    ),
    this.CreateCurveStyle(
        { r: 0.75, g: 1.00, b: 0.35, a: 0.16 },
        { r: 0.75, g: 1.00, b: 0.35, a: 0.04 },
        { r: 0.75, g: 1.00, b: 0.35, a: 0.90, width: 1.5 }
    ),
    this.CreateCurveStyle(
        { r: 0.90, g: 0.45, b: 1.00, a: 0.16 },
        { r: 0.90, g: 0.45, b: 1.00, a: 0.04 },
        { r: 0.90, g: 0.45, b: 1.00, a: 0.90, width: 1.5 }
    )
];

}

SpectrumViewState.prototype.CreateCurveStyle = function(fillTop, fillBottom, outline) {
    return {
        fill: {
            top: fillTop,
            bottom: fillBottom
        },
        outline: outline
    };
};

var spectrumState = new SpectrumViewState();

SpectrumViewState.prototype.LoadFilterColors = function() {
    var dictionary = new Dict();
    dictionary.import_json("Config/ConsolidatorSettings.json");
    var configuration = JSON.parse(dictionary.stringify());
    var filters = configuration.filters || {};
    for (var filterId in filters) {
        if (Object.prototype.hasOwnProperty.call(filters, filterId)) {
            this.filterColors[filterId] = this.ParseColor(filters[filterId].color);
        }
    }
};

SpectrumViewState.prototype.ParseColor = function(value) {
    var text = String(value || "").replace("#", "");
    if (text.length !== 6) return { r: 1, g: 1, b: 1, a: 1 };
    return {
        r: parseInt(text.substr(0, 2), 16) / 255,
        g: parseInt(text.substr(2, 2), 16) / 255,
        b: parseInt(text.substr(4, 2), 16) / 255,
        a: 1
    };
};

spectrumState.LoadFilterColors();
// Shared state, frequency display settings, visual constants, and curve styles.
