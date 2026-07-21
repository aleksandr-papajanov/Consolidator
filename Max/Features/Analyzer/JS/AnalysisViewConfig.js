function AnalysisViewState() {
    this.analysis = {
        windowCount: 0,
        historySeconds: 0,
        metrics: [],
        bands: []
    };
    this.displayMinFrequency = 10;
    this.displayMaxFrequency = 20000;
    this.visualSettings = {
        background: { r: 0.07, g: 0.07, b: 0.07, a: 1.0 },
        metrics: {
            normalizationSensitivity: 1.35,
            grid: { r: 0.25, g: 0.25, b: 0.25, a: 1.0 },
            label: { r: 0.58, g: 0.58, b: 0.58, a: 1.0 },
            current: { r: 1.0, g: 1.0, b: 1.0 },
            reference: { r: 1.0, g: 0.88, b: 0.25 },
            metricColors: [
                { r: 0.30, g: 0.78, b: 1.00, a: 0.86 },
                { r: 1.00, g: 0.55, b: 0.30, a: 0.86 },
                { r: 0.72, g: 0.50, b: 1.00, a: 0.86 },
                { r: 0.38, g: 0.90, b: 0.62, a: 0.86 },
                { r: 0.95, g: 0.80, b: 0.30, a: 0.86 },
                { r: 0.98, g: 0.36, b: 0.55, a: 0.86 },
                { r: 0.35, g: 0.85, b: 0.88, a: 0.86 },
                { r: 0.75, g: 0.75, b: 0.75, a: 0.86 }
            ],
            composite: { r: 1.0, g: 1.0, b: 1.0, a: 0.95 },
            overall: { r: 0.45, g: 1.0, b: 0.48, a: 0.95 }
        }
    };
}

var spectrumState = new AnalysisViewState();
