autowatch = 1;
inlets = 1;
outlets = 0;

function SpectrumViewController() {
}

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

include('JS/AnalysisViewConfig.js');
include('JS/SpectrumViewGeometry.js');
include('JS/SpectrumViewAnalysis.js');

var analysisViewController = new SpectrumViewController();

function paint() {
    try {
        var size = mgraphics.size;
        var color = spectrumState.visualSettings.background;
        mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
        mgraphics.rectangle(0, 0, size[0], size[1]);
        mgraphics.fill();
        analysisViewController.DrawMetricsPage(size[0], size[1]);
    } catch (error) {
        post("Analysis paint error: " + error + "\n");
    }
}

function feature_vector() {
    analysisViewController.FeatureVector.apply(analysisViewController, arguments);
}
function inletassist() {
    assist("feature_vector <windowCount> <historySeconds> <globalMetrics...> <bandMetrics...>");
}
setinletassist(-1, inletassist);
