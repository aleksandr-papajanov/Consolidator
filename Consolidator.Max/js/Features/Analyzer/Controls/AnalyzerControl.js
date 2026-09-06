autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { AnalyzerControlCore } = require("./AnalyzerControlCore.js");
const { AnalyzerLayout } = require("./AnalyzerLayout.js");

class AnalyzerControl extends AnalyzerControlCore
{
    constructor()
    {
        super(Task);
    }
}

function presentation(value) { analyzerControl.applyPresentation(value); }
function paint() { analyzerControl.paint(); }
function onresize() { mgraphics.redraw(); }

function onclick(x, y)
{
    let id = analyzerControl.hitTest(x, y);
    if (!id || (!analyzerControl.canMove(id) && !analyzerControl.canChangeQ(id))) return;
    if (analyzerControl.doubleClick.isDoubleClick(id)) {
        analyzerControl.endGesture();
        analyzerControl.resetInteractionState();
        analyzerControl.emitIntent("filterReset", [id]);
        analyzerControl.requestRedraw();
        return;
    }
    analyzerControl.state.selectedId = id;
    analyzerControl.state.dragging = true;
    analyzerControl.state.lastDragY = y;
    analyzerControl.state.qDragStarted = false;
    analyzerControl.emitIntent("filterSelected", [id]);
    analyzerControl.emitIntent("gestureBegan", [id]);
}

function ondrag(x, y, button, cmd, shift, caps, option, ctrl)
{
    if (button === 0) {
        analyzerControl.endGesture();
        return;
    }
    let state = analyzerControl.state;
    if (!state.dragging) return;
    let alt = Number(option) !== 0;
    if (alt && analyzerControl.canChangeQ(state.selectedId)) {
        if (!state.qDragStarted) {
            state.lastDragY = y;
            state.qDragStarted = true;
            return;
        }
        let delta = state.lastDragY - y;
        analyzerControl.emitIntent("filterQChanged", [state.selectedId, delta]);
        state.lastDragY = y;
        return;
    }
    if (!analyzerControl.canMove(state.selectedId)) return;
    let layout = new AnalyzerLayout(mgraphics.size[0], mgraphics.size[1]);
    let handle = analyzerControl.handleById(state.selectedId);
    let xMinimum = isFinite(Number(handle.xMinimum))
        ? Number(handle.xMinimum) : 0;
    let xMaximum = isFinite(Number(handle.xMaximum))
        ? Number(handle.xMaximum) : 1;
    let yMinimum = isFinite(Number(handle.yMinimum))
        ? Number(handle.yMinimum) : 0;
    let yMaximum = isFinite(Number(handle.yMaximum))
        ? Number(handle.yMaximum) : 1;
    let nextX = Math.max(xMinimum, Math.min(
        xMaximum, (x - layout.left) / layout.width));
    let nextY = Math.max(yMinimum, Math.min(
        yMaximum, (y - layout.top) / layout.height));
    analyzerControl.emitIntent("filterPreview", [state.selectedId, nextX, nextY]);
    analyzerControl.scheduleMove(state.selectedId, nextX, nextY);
    analyzerControl.requestRedraw();
}

function onidleout() { analyzerControl.endGesture(); }

function presentation_begin(mode, enabled, revision, viewKey)
{
    analyzerControl.beginPresentation(mode, enabled, revision, viewKey);
}

function rough_grid(rough, frequencyCount, gainCount, ...values)
{
    analyzerControl.setRoughGrid(rough, frequencyCount, gainCount, values);
}

function applyCurve(name, args, id)
{
    if (analyzerControl.pendingPresentation) {
        analyzerControl.addCurve(name, args, id);
    }
    else {
        analyzerControl.updateCurve(name, args, id);
    }
}

function spectrum(...args) { applyCurve("spectrum", args); }
function reference_spectrum(...args) { applyCurve("reference_spectrum", args); }
function difference_spectrum(...args) { applyCurve("difference_spectrum", args); }
function curve(id, ...args) { applyCurve("curve", args, id); }
function combined(...args) { applyCurve("combined", args); }
function handle(...args) { analyzerControl.addHandle(args); }

function presentation_end()
{
    if (!analyzerControl.pendingPresentation) return;
    analyzerControl.applyPresentation(analyzerControl.pendingPresentation);
    analyzerControl.pendingPresentation = null;
}

function resetInteraction()
{
    analyzerControl.resetInteractionState();
    mgraphics.redraw();
}

function transactionRejected() { resetInteraction(); }
function interactionReset() { resetInteraction(); }

function scope(active, hasColor, red, green, blue, alpha)
{
    analyzerControl.setScope(active, hasColor, red, green, blue, alpha);
}

function notifydeleted() { analyzerControl.destroy(); }

const analyzerControl = new AnalyzerControl();
