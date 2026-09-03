autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { ButtonControlCore } = require("./ButtonControlCore.js");

class ButtonControl extends ButtonControlCore
{
}

function presentation(value) { buttonControl.applyPresentation(value); }
function paint() { buttonControl.paint(); }
function onresize() { mgraphics.redraw(); }
function onclick() { buttonControl.click(); }
function ondrag(x, y, button) {
    if (button === 0) buttonControl.release();
}
function onidleout() { buttonControl.release(); }
function set(value) { buttonControl.setPresentationValue(value); }
function enabled(value) { buttonControl.setPresentationEnabled(value); }
function active(value) { buttonControl.setPresentationActive(value); }
function mode(value) { buttonControl.setPresentationMode(value); }
function label(value) { buttonControl.setPresentationLabel(value); }
function presentation_begin() { buttonControl.beginPresentation(); }
function presentation_end() { buttonControl.endPresentation(); }
function scope(active, hasColor, red, green, blue, alpha) {
    buttonControl.setScope(active, hasColor, red, green, blue, alpha);
}

const buttonControl = new ButtonControl();
