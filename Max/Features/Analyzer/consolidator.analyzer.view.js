autowatch = 1;
inlets = 7;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

include("JS/AnalyzerView/AnalyzerViewConfig.js");
include("JS/AnalyzerView/AnalyzerViewState.js");
include("JS/AnalyzerView/AnalyzerViewGeometry.js");
include("JS/AnalyzerView/AnalyzerViewRenderer.js");
include("JS/AnalyzerView/AnalyzerViewController.js");

var analyzerViewController = new AnalyzerViewController();

function paint() { analyzerViewController.Paint(); }
function list() { analyzerViewController.HandleList(inlet, arrayfromargs(arguments)); }
function anything() { analyzerViewController.HandleAnything(inlet, messagename, arrayfromargs(arguments)); }
function feature_vector() { analyzerViewController.SetFeatureVector(arrayfromargs(arguments)); }
function filter_curve() { analyzerViewController.SetFilterCurve(arrayfromargs(arguments)); }
function curve_settings() { analyzerViewController.SetCurveSettings(arrayfromargs(arguments)); }
function fit_curve() { analyzerViewController.SetCurve("fitCurve", arrayfromargs(arguments)); }
function clear_spectrum() { analyzerViewController.ClearSpectrum(); }
function clear_fit_curve() { analyzerViewController.ClearFitCurve(); }
function clear_analysis() { analyzerViewController.ClearAnalysis(); }
function mode(value) { analyzerViewController.SetMode(String(value)); }
function onclick() { analyzerViewController.OnClick.apply(analyzerViewController, arguments); }
function ondrag() { analyzerViewController.OnDrag.apply(analyzerViewController, arguments); }
function onmouseup() { analyzerViewController.OnMouseUp(); }

function inletassist(index) {
    var descriptions = [
        "Current spectrum in dB",
        "Reference spectrum in dB",
        "fit_curve <dB...>",
        "filter_curve <id> <active> <freq> <gain> <type> <q> <qMin> <qMax> <freqMin> <freqMax> <gainMin> <gainMax> <curve...>; curve_settings ...",
        "Total EQ response in dB",
        "feature_vector <windowCount> <historySeconds> <metrics...>",
        "snapshot 1 host eq ...; mode spectrum|analysis"
    ];
    assist(descriptions[index] || "");
}

function outletassist(index) {
    assist(index === 0 ? "command 1 spectrum <requestId> eq.set_parameter ..." : "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);
