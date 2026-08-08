autowatch = 1;
inlets = 3;
outlets = 4;

include("../JS/ProcessorControllerBase.js");

var controller = new ProcessorControllerBase("saturator");

function inletassist(index) {
    assist([
        "Local saturator saturation-output <ring 1..3> <0..1>|active <0|1>|onsetMatch <0|1>|levelMatch <0|1>, detector_absolute, detector_listen <FilterId> <0|1>; processor_limits, link_color, processor_preview, processor_match_operation <saturator> <onset|level>, processor_bypass_operation <saturator> <0|1>",
        "Host EQ and processor snapshots",
        "target_level <absoluteDb>, processor_telemetry <9 values>"
    ][index] || "");
}

function outletassist(index) {
    if (index === 3) {
        assist("Host-local link dispatch plus match, bypass, and detector-reset group operations");
        return;
    }
    assist([
        "Host commands: eq.*, gain.set_parameter, saturator.*",
        "UI: Dial set|active|activityEnabled|onsetMatchEnabled|levelMatchEnabled|onsetMatch|levelMatch|limits|displayRange|visualization|ringColor; detector preview and ghost marker updates",
        "Diagnostics: error <code>",
        "Live link gesture: processor_parameter_gesture <device> <parameter> <normalizedValue>, processor_match_operation <device> <onset|level>, processor_bypass_operation <device> <0|1>, or processor_detector_reset <device> <FilterId>"
    ][index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function filter() { if (inlet === 0) controller.HandleLocal(["filter"].concat(arrayfromargs(arguments))); }
function input_gain() { if (inlet === 0) controller.HandleLocal(["input_gain"].concat(arrayfromargs(arguments))); }
function saturator() { if (inlet === 0) controller.HandleLocal(["saturator"].concat(arrayfromargs(arguments))); }
function output_gain() { if (inlet === 0) controller.HandleLocal(["output_gain"].concat(arrayfromargs(arguments))); }
function snapshot() { if (inlet === 1) controller.HandleSnapshot(["snapshot"].concat(arrayfromargs(arguments))); }
function event() {}
function status() {}
function target_level(value) { if (inlet === 2) controller.HandleTargetLevel(value); }
function processor_telemetry() { if (inlet === 2) controller.HandleProcessorTelemetry(arrayfromargs(arguments)); }
function processor_limits(device, parameter, minimum, maximum) {
    if (inlet === 0) controller.HandleProcessorLimits(String(device), String(parameter), Number(minimum), Number(maximum));
}
function link_color(linkId, red, green, blue, alpha) {
    if (inlet === 0) controller.HandleLinkColor(String(linkId), Number(red), Number(green), Number(blue), Number(alpha));
}
function processor_preview(device, parameter, absoluteValue) {
    if (inlet === 0) controller.HandleProcessorPreview(String(device), String(parameter), Number(absoluteValue));
}
function processor_match_operation(device, operation) {
    if (inlet === 0) controller.HandleGroupMatch(String(device), String(operation));
}
function processor_bypass_operation(device, bypass) {
    if (inlet === 0) controller.HandleGroupBypass(String(device), Number(bypass));
}
function processor_detector_reset(device, FilterId) {
    if (inlet === 0) controller.ResetDetector(String(device), Number(FilterId));
}
function list() {
    var values = arrayfromargs(arguments);
    if (inlet === 1 && values.length && String(values[0]) === "snapshot") controller.HandleSnapshot(values);
}

function loadbang() {
    controller.Initialize();
}

function notifydeleted() { controller.Dispose(); }
