function SpectrumViewModel() {}

SpectrumViewModel.prototype.Build = function(state) {
    return {
        mode: state.mode,
        scaleIndex: state.scaleIndex,
        eqBypass: state.eqBypass,
        operationAvailability: state.operationAvailability,
        currentCurve: state.currentCurve,
        referenceCurve: state.referenceCurve,
        fitCurve: state.fitCurve,
        totalCurve: state.totalCurve,
        filterCurves: state.filterCurves,
        handles: state.handles,
        linkedHandles: state.linkedHandles,
        linkId: state.linkId,
        linkColor: state.linkColor
    };
};
