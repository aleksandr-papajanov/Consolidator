var curves = [[], [], [], [], []];
var filterCurves = {};
var handles = [];
var draggedHandle = null;
var draggedHandleSlot = null;
var selectionCandidates = [];
var selectionIndex = -1;
var selectionX = 0;
var selectionY = 0;
var selectedHandleSlot = null;
var clickWasRepeat = false;
var clickMoved = false;
var draggingWithAlt = false;
var dragStartX = 0;
var dragStartY = 0;
var dragStartFrequency = 0;
var dragStartGain = 0;
var dragStartQNormalized = 0;
var qSensitivity = 1.5;

var minDb = -15;
var maxDb = 15;
var dbRangePresets = [
    { min: -15, max: 15, label: "15 dB" },
    { min: -30, max: 30, label: "30 dB" }
];
var dbRangeIndex = 1;
var displayMinFrequency = 10;
var displayMaxFrequency = 20000;
var curveMinFrequency = 20;

var frequencyBands = [
    { min: 10, max: 100, width: 0.31 },
    { min: 100, max: 1000, width: 0.31 },
    { min: 1000, max: 10000, width: 0.31 },
    { min: 10000, max: 20000, width: 0.07 }
];

var frequencyLabels = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000];
var majorFrequencies = [100, 1000, 10000];
var minorFrequencies = [
    10, 20, 30, 40, 50, 60, 70, 80, 90,
    200, 300, 400, 500, 600, 700, 800, 900,
    2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000
];

var visualSettings = {
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
var smoothing = 0.75;

var styles = [
    createCurveStyle(
        visualSettings.reference,
        { r: 1.00, g: 1.00, b: 1.00, a: 0.1 },
        null
    ),
    createCurveStyle(
        visualSettings.target,
        { r: 1.00, g: 0.88, b: 0.25, a: 0.1 },
        null
    ),
    createCurveStyle(
        { r: 1.00, g: 1.00, b: 1.00, a: 0.00 },
        { r: 1.00, g: 1.00, b: 1.00, a: 0.00 },
        { r: visualSettings.difference.r, g: visualSettings.difference.g, b: visualSettings.difference.b,
            a: visualSettings.difference.a, width: visualSettings.totalLineWidth }
    ),
    createCurveStyle(
        { r: 0.75, g: 1.00, b: 0.35, a: 0.16 },
        { r: 0.75, g: 1.00, b: 0.35, a: 0.04 },
        { r: 0.75, g: 1.00, b: 0.35, a: 0.90, width: 1.5 }
    ),
    createCurveStyle(
        { r: 0.90, g: 0.45, b: 1.00, a: 0.16 },
        { r: 0.90, g: 0.45, b: 1.00, a: 0.04 },
        { r: 0.90, g: 0.45, b: 1.00, a: 0.90, width: 1.5 }
    )
];

function createCurveStyle(fillTop, fillBottom, outline) {
    return {
        fill: {
            top: fillTop,
            bottom: fillBottom
        },
        outline: outline
    };
}
// Shared state, frequency display settings, visual constants, and curve styles.
