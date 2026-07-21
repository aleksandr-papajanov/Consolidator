function ProcessorMetersState() {
    this.processorTelemetry = {
        initialized: false,
        compressorReductionDb: 0,
        saturationNonlinearRatio: 0,
        saturationLevelDeltaDb: 0
    };
    this.visualSettings = {
        processorTelemetry: {
            smoothing: 0.68,
            maximumReductionDb: 20,
            maximumSaturationPercent: 40,
            background: { r: 0.055, g: 0.055, b: 0.055, a: 1.0 },
            separator: { r: 0.28, g: 0.28, b: 0.28, a: 1.0 },
            scale: { r: 0.42, g: 0.42, b: 0.42, a: 0.9 },
            needle: { r: 0.10, g: 0.78, b: 0.92, a: 1.0 },
            text: { r: 0.68, g: 0.68, b: 0.68, a: 0.95 }
        }
    };
}

var spectrumState = new ProcessorMetersState();
