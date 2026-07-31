include("Spectrum/SpectrumViewModel.js");
include("Analysis/AnalysisViewModel.js");

function AnalyzerViewModel() {
    this.spectrumViewModel = new SpectrumViewModel();
    this.analysisViewModel = new AnalysisViewModel();
}

AnalyzerViewModel.prototype.Build = function(state) {
    return {
        mode: state.mode,
        spectrum: this.spectrumViewModel.Build(state),
        analysis: this.analysisViewModel.Build(state)
    };
};
