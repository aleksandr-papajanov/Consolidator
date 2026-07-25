autowatch = 1;
inlets = 6;
outlets = 1;

function SpectrumViewController() {
}

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

include('JS/SpectrumViewConfig.js');
include('JS/SpectrumViewGeometry.js');
include('JS/SpectrumViewCurves.js');
include('JS/SpectrumViewInput.js');

var spectrumViewController = new SpectrumViewController();

function inletassist(index) {
    var descriptions = [
        "Current signal spectrum in dB; view commands: clear, range <min> <max>, range_mode <index>, toggle_range, smooth <0..1>, q_sensitivity <0..1>",
        "Reference signal spectrum in dB",
        "difference <dB...>; fit_curve <dB...>; clear_fit_curve",
        "curve_settings <minHz> <maxHz> <pointCount>; filter_curve <fields...>",
        "Total EQ response curve in dB",
        "snapshot 1 host eq ... from DeviceHost"
    ];
    assist(descriptions[index] || "");
}

function outletassist(index) {
    assist(index === 0
        ? "command 1 spectrum <requestId> eq.set_parameter ... for DeviceHost"
        : "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function paint() {
    try {
        spectrumViewController.Paint();
    } catch (error) {
        post("Spectrum paint error: " + error + "\n");
    }
}
function list() {
    if (inlet === 5) spectrumViewController.HandleBusMessage(arrayfromargs(arguments));
    else spectrumViewController.List.apply(spectrumViewController, arguments);
}
function filter_curve() { spectrumViewController.FilterCurve.apply(spectrumViewController, arguments); }
function fit_curve() { spectrumViewController.SetFitCurve(arrayfromargs(arguments)); }
function difference() { spectrumViewController.SetDifference(arrayfromargs(arguments)); }
function curve_settings() { spectrumViewController.CurveSettings.apply(spectrumViewController, arguments); }
function onclick() { spectrumViewController.OnClick.apply(spectrumViewController, arguments); }
function ondrag() { spectrumViewController.OnDrag.apply(spectrumViewController, arguments); }
function onmouseup() { spectrumViewController.OnMouseUp.apply(spectrumViewController, arguments); }
function clear() { spectrumViewController.Clear(); }
function clear_fit_curve() { spectrumViewController.ClearDifference(); }
function range() { spectrumViewController.Range.apply(spectrumViewController, arguments); }
function range_mode() { spectrumViewController.RangeMode.apply(spectrumViewController, arguments); }
function toggle_range() { spectrumViewController.ToggleRange(); }
function smooth() { spectrumViewController.Smooth.apply(spectrumViewController, arguments); }
function q_sensitivity() { spectrumViewController.QSensitivity.apply(spectrumViewController, arguments); }
function anything() {
    if (inlet === 5 && messagename === "snapshot") {
        spectrumViewController.HandleBusMessage(["snapshot"].concat(arrayfromargs(arguments)));
    }
}
