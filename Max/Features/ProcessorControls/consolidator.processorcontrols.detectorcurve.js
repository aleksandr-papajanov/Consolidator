autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

include("../../Shared/Interface/DetectorCurve/DetectorCurveViewModel.js");
include("../../Shared/Interface/DetectorCurve/DetectorCurveRenderer.js");
include("../../Shared/Interface/DetectorCurve/DetectorCurveController.js");
include("../../Shared/Configuration/FilterDefinitions.js");

var detectorCurveViewModel = new DetectorCurveViewModel();
var detectorCurveRenderer = new DetectorCurveRenderer();
var detectorCurveController = new DetectorCurveController(
    detectorCurveViewModel,
    detectorCurveRenderer
);

detectorCurveViewModel.SetCatalogDefinitions(FilterDefinitionCatalog.Detector());

function paint() {
    detectorCurveRenderer.Paint(detectorCurveViewModel);
}

function detector(FilterId, bypass, gainDb, frequencyHz, q) {
    if (detectorCurveViewModel.SetDetector(FilterId, bypass, gainDb, frequencyHz, q)) {
        mgraphics.redraw();
    }
}

function definition() {
    var values = arrayfromargs(arguments);
    if (values.length >= 11) {
        detectorCurveViewModel.SetDefinition.apply(
            detectorCurveViewModel,
            values
        );
        mgraphics.redraw();
    }
}

function listen(FilterId, enabled) {
    detectorCurveController.SetListen(FilterId, enabled);
}

function link_color(linkId, red, green, blue, alpha) {
    detectorCurveViewModel.SetLinkColor(linkId, red, green, blue, alpha);
    mgraphics.redraw();
}
function limits(FilterId, parameter, minimum, maximum) {
    detectorCurveViewModel.SetLimit(FilterId, parameter, minimum, maximum);
    mgraphics.redraw();
}
function preview(FilterId, parameter, value) {
    detectorCurveViewModel.SetPreview(FilterId, parameter, value);
    mgraphics.redraw();
}
function reset(FilterId) {
    detectorCurveViewModel.Reset(FilterId);
    mgraphics.redraw();
}
function onresize() {
    mgraphics.redraw();
}

function onclick(x, y, button, mod1, shift, caps, option, control) {
    detectorCurveController.BeginDrag(x, y, option, control);
}

function ondrag(x, y, button, mod1, shift, caps, option) {
    if (button === 0) {
        detectorCurveController.EndDrag();
        return;
    }
    detectorCurveController.Drag(x, y, button, option);
}

function inletassist(index) {
    assist(index === 0
        ? "detector <FilterId> <bypass> <gainDb> <frequencyHz> <q>; definition, listen, limits, preview, link_color; output detector_absolute or detector_listen"
        : "");
}

setinletassist(-1, inletassist);
