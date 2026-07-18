autowatch = 1;
inlets = 5;
outlets = 1;

function SpectrumViewController() {
}

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

include('SpectrumViewConfig.js');
include('SpectrumViewGeometry.js');
include('SpectrumViewCurves.js');
include('SpectrumViewInput.js');

var spectrumViewController = new SpectrumViewController();

function inletassist(index) {
    var descriptions = [
        "Current signal spectrum in dB",
        "Reference signal spectrum in dB",
        "Difference spectrum in dB",
        "Individual filter curves and handles",
        "Total EQ response curve in dB"
    ];
    assist(descriptions[index] || "");
}

function outletassist(index) {
    assist(index === 0
        ? "Filter edit command for the message bus"
        : "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function paint() {
    try {
        spectrumViewController.paint();
    } catch (error) {
        post("SpectrumView paint error: " + error + "\n");
    }
}
function list() { spectrumViewController.list.apply(spectrumViewController, arguments); }
function handle() { spectrumViewController.handle.apply(spectrumViewController, arguments); }
function filter_curve() { spectrumViewController.filter_curve.apply(spectrumViewController, arguments); }
function onclick() { spectrumViewController.onclick.apply(spectrumViewController, arguments); }
function ondrag() { spectrumViewController.ondrag.apply(spectrumViewController, arguments); }
function onmouseup() { spectrumViewController.onmouseup.apply(spectrumViewController, arguments); }
function clear() { spectrumViewController.clear(); }
function clear_difference() { spectrumViewController.clear_difference(); }
function target_size() { spectrumViewController.target_size.apply(spectrumViewController, arguments); }
function range() { spectrumViewController.range.apply(spectrumViewController, arguments); }
function range_mode() { spectrumViewController.range_mode.apply(spectrumViewController, arguments); }
function toggle_range() { spectrumViewController.toggle_range(); }
function smooth() { spectrumViewController.smooth.apply(spectrumViewController, arguments); }
function q_sensitivity() { spectrumViewController.q_sensitivity.apply(spectrumViewController, arguments); }
