class AnalyzerPresentation
{
    constructor()
    {
        this.enabled = true;
        this.mode = "equalizer";
        this.spectrum = null;
        this.referenceSpectrum = null;
        this.differenceSpectrum = null;
        this.curves = [];
        this.combinedCurve = null;
        this.allBanksCurve = null;
        this.handles = [];
        this.parameterRevision = 0;
        this.viewKey = "";
        this.scopeActive = false;
        this.scopeColor = null;
    }
}

module.exports = {
    AnalyzerPresentation: AnalyzerPresentation
};
