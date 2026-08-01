autowatch = 1;
inlets = 7;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

include("../../Shared/Interface/Analyzer/AnalyzerViewController.js");

var analyzerViewController = new AnalyzerViewController();

function paint() { analyzerViewController.Paint(); }
function list() { analyzerViewController.HandleList(inlet, arrayfromargs(arguments)); }
function anything() { analyzerViewController.HandleAnything(inlet, messagename, arrayfromargs(arguments)); }
function feature_vector() { analyzerViewController.SetFeatureVector(arrayfromargs(arguments)); }
function filter_curve() { analyzerViewController.SetFilterCurve(arrayfromargs(arguments)); }
function curve_settings() { analyzerViewController.SetCurveSettings(arrayfromargs(arguments)); }
function fit_curve() { analyzerViewController.SetCurve("fitCurve", arrayfromargs(arguments)); }
function clear_spectrum() {
    if (inlet === 0) analyzerViewController.ClearCurrentCurve();
    else if (inlet === 1) analyzerViewController.ClearReferenceCurve();
    else analyzerViewController.ClearSpectrum();
}
function clear_fit_curve() { analyzerViewController.ClearFitCurve(); }
function clear_analysis() { analyzerViewController.ClearAnalysis(); }
function mode(value) { analyzerViewController.SetMode(String(value)); }
function onclick() { analyzerViewController.OnClick.apply(analyzerViewController, arguments); }
function ondblclick() { analyzerViewController.OnDoubleClick.apply(analyzerViewController, arguments); }
function ondrag(x, y, button, cmd, shift, capslock, option) {
    if (Number(button) === 0) {
        analyzerViewController.OnMouseUp();
        return;
    }
    analyzerViewController.OnDrag(x, y, button, cmd, shift, capslock, option);
}
function onmouseup() { analyzerViewController.OnMouseUp(); }
function notifydeleted() { analyzerViewController.Dispose(); }

function inletassist(index) {
    var descriptions = [
        "Current spectrum in dB",
        "Reference spectrum in dB",
        "fit_curve <dB...>",
        "filter_curve <id> <active> <freq> <gain> <type> <q> [<curve...>]; link_color <linkId|-> <r> <g> <b> <a>; eq_link_preview <linkId> <sourceId> <filterId> <active> <freq> <gain> <q> <type>; curve_settings ...",
        "Total EQ response in dB",
        "feature_vector <windowCount> <historySeconds> <metrics...>",
        "snapshot 1 host eq ...; mode spectrum|analysis; curve_settings <minimumHz> <maximumHz> <pointCount> <minimumSpectrumDb> <maximumSpectrumDb>; top UI FFT|ANALYSIS|RANGE; bottom UI B|R|JOIN|COMMIT|MATCH EQ|CLEAR"
    ];
    assist(descriptions[index] || "");
}

function outletassist(index) {
    assist(index === 0
        ? "command 1 spectrum <requestId> eq.set_parameter|eq.set_bypass|eq.reset_filter; bank.action <join|commit|reset|bypass>"
        : "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);
