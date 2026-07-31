function AnalysisViewModel() {}

AnalysisViewModel.prototype.Build = function(state) {
    return {
        mode: state.mode,
        scaleIndex: state.scaleIndex,
        eqBypass: state.eqBypass,
        operationAvailability: state.operationAvailability,
        analysis: state.analysis
    };
};
