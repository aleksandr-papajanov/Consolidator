autowatch = 1;
inlets = 1;
outlets = 1;
mgraphics.init();

include("../../../Shared/Interface/Routing/RoutingControl.js");

var routingControl = new RoutingControl();

function source_items() { routingControl.SetItems("source", arrayfromargs(arguments)); }
function channel_items() { routingControl.SetItems("channel", arrayfromargs(arguments)); }
function source_selection(value) { routingControl.SetSelection("source", value); }
function channel_selection(value) { routingControl.SetSelection("channel", value); }
function source_enabled(value) { routingControl.SetEnabled("source", value); }
function channel_enabled(value) { routingControl.SetEnabled("channel", value); }
function paint() { routingControl.Paint(); }
function onclick(x, y) { routingControl.HandleClick(x, y); }
function onwheel(x, y, scrollx, scrolly, cmd, shift, capslock, option, ctrl) {
    routingControl.HandleWheel(x, y, scrolly);
}
