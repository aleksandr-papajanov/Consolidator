include("Spectrum/SpectrumRenderer.js");
include("Analysis/AnalysisRenderer.js");

function AnalyzerViewRenderer() {
    this.spectrumRenderer = new SpectrumRenderer();
    this.analysisRenderer = new AnalysisRenderer();
}

AnalyzerViewRenderer.prototype.Paint = function(state) {
    var size = mgraphics.size;
    if (state.mode === "analysis") {
        this.analysisRenderer.Paint(state.analysis, size[0], size[1]);
    } else {
        this.spectrumRenderer.Paint(state.spectrum, size[0], size[1]);
    }
};

var analyzerViewRenderer = new AnalyzerViewRenderer();
