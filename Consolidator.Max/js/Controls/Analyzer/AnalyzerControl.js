autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { AnalyzerViewState } = require("./AnalyzerViewState.js");
const { AnalyzerLayout } = require("./AnalyzerLayout.js");
const { AnalyzerRenderer } = require("./AnalyzerRenderer.js");

const ANALYZER_REDRAW_INTERVAL_MS = 16;
const ANALYZER_MOVE_INTERVAL_MS = 33;

class AnalyzerControl
{
    constructor()
    {
        this.presentation = null;
        this.pendingPresentation = null;
        this.state = new AnalyzerViewState();
        this.renderer = new AnalyzerRenderer();
        this.parameterRevision = 0;
        this.viewKey = "";
        this.redrawScheduled = false;
        this.redrawTimer = new Task(() => {
            this.redrawScheduled = false;
            mgraphics.redraw();
        }, this);
        this.moveScheduled = false;
        this.moveTimer = new Task(() => {
            this.moveScheduled = false;
            this.flushMove();
        }, this);
    }

    resetInteractionState()
    {
        this.state.preview = {};
        this.state.pendingMove = null;
        this.state.dragging = false;
        this.state.selectedId = 0;
    }

    applyPresentation(presentation)
    {
        this.presentation = presentation;
        const parameterRevision = Number(presentation.parameterRevision);
        const viewKey = presentation.viewKey === undefined
            ? "" : String(presentation.viewKey);
        if (parameterRevision !== this.parameterRevision ||
                viewKey !== this.viewKey) {
            this.resetInteractionState();
            this.parameterRevision = parameterRevision;
            this.viewKey = viewKey;
        }
        if (!presentation.handles || presentation.handles.length === 0) {
            this.resetInteractionState();
        }
        (presentation.handles || []).forEach((handle) => {
            const preview = this.state.preview[handle.id];
            if (preview && Math.abs(preview.x - handle.frequency) < 0.0001 &&
                    Math.abs(preview.y - handle.gain) < 0.0001) {
                delete this.state.preview[handle.id];
            }
        });
        this.requestRedraw();
    }

    beginPresentation(mode, enabled, parameterRevision, viewKey)
    {
        let current = this.presentation || {};
        this.pendingPresentation = {
            mode: String(mode),
            enabled: Number(enabled) !== 0,
            parameterRevision: Number(parameterRevision),
            viewKey: viewKey === undefined ? "" : String(viewKey),
            spectrum: current.spectrum || null,
            referenceSpectrum: current.referenceSpectrum || null,
            differenceSpectrum: current.differenceSpectrum || null,
            curves: (current.curves || []).slice(0),
            combinedCurve: current.combinedCurve || null,
            allBanksCurve: current.allBanksCurve || null,
            handles: []
        };
    }

    addCurve(name, args, id)
    {
        if (!this.pendingPresentation) return;
        let presentation = this.pendingPresentation;
        let curve = {
            active: Number(args[0]) !== 0,
            values: args.slice(1)
        };
        if (name === "spectrum") presentation.spectrum = curve;
        else if (name === "reference_spectrum") {
            presentation.referenceSpectrum = curve;
        }
        else if (name === "difference_spectrum") {
            presentation.differenceSpectrum = curve;
        }
        else if (name === "combined") presentation.combinedCurve = curve;
        else if (name === "all_banks") presentation.allBanksCurve = curve;
        else presentation.curves.push({
            id: Number(id),
            active: curve.active,
            values: curve.values
        });
    }

    updateCurve(name, args, id)
    {
        if (!this.presentation) return;
        let curve = {
            active: Number(args[0]) !== 0,
            values: args.slice(1)
        };
        if (name === "spectrum") this.presentation.spectrum = curve;
        else if (name === "reference_spectrum") {
            this.presentation.referenceSpectrum = curve;
        }
        else if (name === "difference_spectrum") {
            this.presentation.differenceSpectrum = curve;
        }
        else if (name === "combined") this.presentation.combinedCurve = curve;
        else if (name === "all_banks") this.presentation.allBanksCurve = curve;
        else if (name === "curve") {
            for (let index = 0; index < this.presentation.curves.length; index += 1) {
                if (this.presentation.curves[index].id === Number(id)) {
                    this.presentation.curves[index] = {
                        id: Number(id),
                        active: curve.active,
                        values: curve.values
                    };
                    this.requestRedraw();
                    return;
                }
            }
            this.presentation.curves.push({
                id: Number(id),
                active: curve.active,
                values: curve.values
            });
        }
        else return;
        this.requestRedraw();
    }

    requestRedraw()
    {
        if (this.redrawScheduled) return;
        this.redrawScheduled = true;
        this.redrawTimer.schedule(ANALYZER_REDRAW_INTERVAL_MS);
    }

    addHandle(args)
    {
        if (!this.pendingPresentation) return;
        let presentation = this.pendingPresentation;
        let handle = {
            id: Number(args[0]),
            frequency: Number(args[1]),
            gain: Number(args[2]),
            enabled: Number(args[3]) !== 0,
            capabilities: {
                frequency: Number(args[4]) !== 0,
                gain: Number(args[5]) !== 0,
                q: Number(args[6]) !== 0
            },
            selected: Number(args[7]) !== 0,
            xMinimum: isFinite(Number(args[8])) ? Number(args[8]) : 0,
            xMaximum: isFinite(Number(args[9])) ? Number(args[9]) : 1,
            yMinimum: isFinite(Number(args[10])) ? Number(args[10]) : 0,
            yMaximum: isFinite(Number(args[11])) ? Number(args[11]) : 1
        };
        for (let index = 0; index < presentation.handles.length; index += 1) {
            if (presentation.handles[index].id === handle.id) {
                presentation.handles[index] = handle;
                return;
            }
        }
        presentation.handles.push(handle);
    }

    paint()
    {
        this.renderer.paint(mgraphics, this.presentation,
            new AnalyzerLayout(mgraphics.size[0], mgraphics.size[1]), this.state);
    }

    hitTest(x, y)
    {
        if (!this.presentation) return 0;
        let layout = new AnalyzerLayout(mgraphics.size[0], mgraphics.size[1]);
        let best = 0;
        let distance = 12;
        (this.presentation.handles || []).forEach((handle) => {
            let hx = layout.left + layout.width * handle.frequency;
            let hy = layout.top + layout.height * handle.gain;
            let next = Math.sqrt(Math.pow(x - hx, 2) + Math.pow(y - hy, 2));
            if (next < distance) { distance = next; best = handle.id; }
        });
        return best;
    }

    handleById(id)
    {
        let handles = this.presentation && this.presentation.handles || [];
        for (let index = 0; index < handles.length; index += 1) {
            if (handles[index].id === id) return handles[index];
        }
        return null;
    }

    canMove(id)
    {
        let handle = this.handleById(id);
        let capabilities = handle && handle.capabilities;
        return Boolean(capabilities &&
            (capabilities.frequency || capabilities.gain));
    }

    canChangeQ(id)
    {
        let handle = this.handleById(id);
        return Boolean(handle && handle.capabilities && handle.capabilities.q);
    }

    emitIntent(name, values)
    {
        outlet(0, [name].concat(values || []));
    }

    scheduleMove(id, x, y)
    {
        this.state.pendingMove = [id, x, y];
        if (this.moveScheduled) return;
        this.moveScheduled = true;
        this.moveTimer.schedule(ANALYZER_MOVE_INTERVAL_MS);
    }

    flushMove()
    {
        if (!this.state.pendingMove) return;
        let move = this.state.pendingMove;
        this.state.pendingMove = null;
        this.emitIntent("filterMoved", move);
    }

    endGesture()
    {
        if (!this.state.dragging) return;
        if (this.moveScheduled) {
            this.moveTimer.cancel();
            this.moveScheduled = false;
        }
        this.flushMove();
        this.state.dragging = false;
        this.emitIntent("gestureEnded", [this.state.selectedId]);
    }

    destroy()
    {
        if (this.redrawScheduled) {
            this.redrawTimer.cancel();
            this.redrawScheduled = false;
        }
        if (this.moveScheduled) {
            this.moveTimer.cancel();
            this.moveScheduled = false;
        }
        this.state.pendingMove = null;
    }
}

function presentation(value) { analyzerControl.applyPresentation(value); }
function paint() { analyzerControl.paint(); }
function onresize() { mgraphics.redraw(); }
function onclick(x, y) {
    let id = analyzerControl.hitTest(x, y);
    if (!id || !analyzerControl.canMove(id)) return;
    analyzerControl.state.selectedId = id;
    analyzerControl.state.dragging = true;
    analyzerControl.emitIntent("filterSelected", [id]);
    analyzerControl.emitIntent("gestureBegan", [id]);
}
function ondrag(x, y, button) {
    if (button === 0) {
        analyzerControl.endGesture();
        return;
    }
    if (!analyzerControl.state.dragging) return;
    if (!analyzerControl.canMove(analyzerControl.state.selectedId)) return;
    let layout = new AnalyzerLayout(mgraphics.size[0], mgraphics.size[1]);
    let handle = analyzerControl.handleById(
        analyzerControl.state.selectedId);
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
    analyzerControl.state.preview[analyzerControl.state.selectedId] = {
        x: nextX,
        y: nextY
    };
    analyzerControl.emitIntent("filterPreview", [
        analyzerControl.state.selectedId,
        nextX,
        nextY
    ]);
    analyzerControl.scheduleMove(
        analyzerControl.state.selectedId,
        nextX,
        nextY
    );
    analyzerControl.requestRedraw();
}

function onwheel(x, y, delta, mod1, shift, caps, opt, mod2) {
    let id = analyzerControl.hitTest(x, y);
    if (!id || !analyzerControl.canChangeQ(id)) return;
    analyzerControl.emitIntent("filterQChanged", [id, Number(delta) * 0.05]);
}
function onidleout() { analyzerControl.endGesture(); }

function presentation_begin(mode, enabled, parameterRevision, viewKey) {
    analyzerControl.beginPresentation(
        mode, enabled, parameterRevision, viewKey);
}

function spectrum(...args) {
    if (analyzerControl.pendingPresentation) {
        analyzerControl.addCurve("spectrum", args);
    } else {
        analyzerControl.updateCurve("spectrum", args);
    }
}

function reference_spectrum(...args) {
    if (analyzerControl.pendingPresentation) {
        analyzerControl.addCurve("reference_spectrum", args);
    } else {
        analyzerControl.updateCurve("reference_spectrum", args);
    }
}

function difference_spectrum(...args) {
    if (analyzerControl.pendingPresentation) {
        analyzerControl.addCurve("difference_spectrum", args);
    } else {
        analyzerControl.updateCurve("difference_spectrum", args);
    }
}

function curve(id, ...args) {
    if (analyzerControl.pendingPresentation) {
        analyzerControl.addCurve("curve", args, id);
    } else {
        analyzerControl.updateCurve("curve", args, id);
    }
}

function combined(...args) {
    if (analyzerControl.pendingPresentation) {
        analyzerControl.addCurve("combined", args);
    } else {
        analyzerControl.updateCurve("combined", args);
    }
}

function all_banks(...args) {
    if (analyzerControl.pendingPresentation) {
        analyzerControl.addCurve("all_banks", args);
    } else {
        analyzerControl.updateCurve("all_banks", args);
    }
}

function handle(...args) {
    analyzerControl.addHandle(args);
}

function presentation_end() {
    if (!analyzerControl.pendingPresentation) return;
    analyzerControl.applyPresentation(analyzerControl.pendingPresentation);
    analyzerControl.pendingPresentation = null;
}

function transactionRejected() {
    analyzerControl.resetInteractionState();
    mgraphics.redraw();
}

function notifydeleted() {
    analyzerControl.destroy();
}

const analyzerControl = new AnalyzerControl();
