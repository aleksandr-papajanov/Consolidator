autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { MultiValueToggleControlCore } = require(
    "./MultiValueToggleControlCore.js");

class MultiValueToggleControl extends MultiValueToggleControlCore
{
}

function presentation(value) {
    multiValueToggleControl.value = Number(value.value);
    multiValueToggleControl.values = value.values || [];
    multiValueToggleControl.redraw();
}
function presentation_begin() { multiValueToggleControl.inPresentation = true; }
function presentation_end() {
    multiValueToggleControl.inPresentation = false;
    mgraphics.redraw();
}
function paint() { multiValueToggleControl.paint(); }
function onclick(x, y) { multiValueToggleControl.beginGesture(y); }
function ondrag(x, y, button) {
    if (button === 0) multiValueToggleControl.endGesture();
    else multiValueToggleControl.drag(y);
}
function onmousemove(x, y) { multiValueToggleControl.updateHover(x, y); }
function onidleout() {
    multiValueToggleControl.endGesture();
    multiValueToggleControl.clearHover();
}
function set(value) {
    multiValueToggleControl.value = Number(value);
    multiValueToggleControl.redraw();
}
function values(...items) {
    multiValueToggleControl.values = items.map(String);
    multiValueToggleControl.redraw();
}
function enabled(value) {
    multiValueToggleControl.enabled = Number(value) !== 0;
    multiValueToggleControl.redraw();
}
function active(value) {
    multiValueToggleControl.active = Number(value) !== 0;
    multiValueToggleControl.redraw();
}
function scope(active, hasColor, red, green, blue, alpha) {
    multiValueToggleControl.scopeActive = Number(active) !== 0;
    multiValueToggleControl.scopeColor = Number(hasColor) !== 0
        ? [Number(red), Number(green), Number(blue), Number(alpha)] : null;
    multiValueToggleControl.redraw();
}

const multiValueToggleControl = new MultiValueToggleControl();
