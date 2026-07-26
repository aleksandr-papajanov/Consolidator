function AnalyzerViewState() {
    this.mode = "spectrum";
    this.scaleIndex = 1;
    this.currentCurve = [];
    this.referenceCurve = [];
    this.fitCurve = [];
    this.totalCurve = [];
    this.filterCurves = {};
    this.handles = [];
    this.selectedBankId = 1;
    this.selectedHandleId = null;
    this.dragHandleId = null;
    this.dragStart = null;
    this.curveSettings = { minimumHz: 20, maximumHz: 20000, pointCount: 0 };
    this.analysis = { metrics: [], bands: [], windowCount: 0, historySeconds: 0 };
    this.requestId = 0;
}
