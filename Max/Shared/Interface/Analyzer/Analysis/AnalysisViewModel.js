function AnalysisViewModel() {}

AnalysisViewModel.prototype.Build = function(state) {
    return {
        mode: state.mode,
        scaleIndex: state.scaleIndex,
        eqBypass: state.eqBypass,
        operationAvailability: state.operationAvailability,
        linkId: state.linkId,
        linkColor: state.linkColor,
        analysis: state.analysis
    };
};
