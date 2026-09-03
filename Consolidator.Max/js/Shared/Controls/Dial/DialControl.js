autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { DialControlCore } = require("./DialControlCore.js");

function dialArgument(index, fallback)
{
    return jsarguments.length > index ? jsarguments[index] : fallback;
}

const DialControlConfiguration = {
    label: String(dialArgument(1, "")),
    minimum: Number(dialArgument(2, 0)),
    maximum: Number(dialArgument(3, 1)),
    logarithmic: Number(dialArgument(4, 0)) !== 0,
    scale: Number(dialArgument(5, 1)),
    decimals: Math.max(0, Math.floor(Number(dialArgument(6, 2)))),
    suffix: String(dialArgument(7, ""))
};

class DialControl extends DialControlCore
{
    constructor()
    {
        super(DialControlConfiguration, Task);
    }
}

function presentation(value) { dialControl.applyPresentation(value); }
function presentation_begin() { dialControl.beginPresentation(); }
function presentation_end() { dialControl.endPresentation(); }
function paint() { dialControl.paint(); }
function onresize() { mgraphics.redraw(); }
function onclick(x, y) { dialControl.beginGesture(0, y); }

function ondrag(x, y, button)
{
    if (button === 0) dialControl.endGesture();
    else dialControl.drag(y);
}

function onidleout() { dialControl.endGesture(); }
function setValue(index, value) {
    dialControl.setValue(Number(index), Number(value), false);
}
function resetValue(index) { dialControl.resetValue(Number(index)); }
function transactionRejected() { dialControl.rejectTransaction(); }
function set(index, value) {
    dialControl.setPresentationValue(Number(index), Number(value));
}
function limits(index, minimum, maximum) {
    dialControl.setPresentationLimits(
        Number(index), Number(minimum), Number(maximum));
}
function enabled(value) {
    dialControl.presentation.enabled = Number(value) !== 0;
    dialControl.requestRedraw();
}
function active(value) {
    dialControl.presentation.active = Number(value) !== 0;
    dialControl.requestRedraw();
}
function scope(active, hasColor, red, green, blue, alpha) {
    dialControl.setScope(active, hasColor, red, green, blue, alpha);
}
function notifydeleted() { dialControl.destroy(); }

const dialControl = new DialControl();
