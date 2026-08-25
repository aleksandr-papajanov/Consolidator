autowatch = 1;
inlets = 1;
outlets = 1;
include("Project:/js/Controls/Analyzer/AnalyzerViewState.js");
include("Project:/js/Controls/Analyzer/AnalyzerLayout.js");
include("Project:/js/Controls/Analyzer/AnalyzerRenderer.js");

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

function AnalyzerControl() {
    this.presentation = null;
    this.pendingPresentation = null;
    this.state = new AnalyzerViewState();
    this.renderer = new AnalyzerRenderer();
    this.parameterRevision = 0;
    this.viewKey = "";
    this.spectrumRedrawTimer = null;
}

AnalyzerControl.prototype.resetInteractionState = function () {
    this.state.preview = {};
    this.state.pendingMove = null;
    this.state.dragging = false;
    this.state.selectedId = 0;
};

AnalyzerControl.prototype.applyPresentation = function (presentation) {
    this.presentation = presentation;
    var self = this;
    var parameterRevision = Number(presentation.parameterRevision);
    var viewKey = presentation.viewKey === undefined
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
    (presentation.handles || []).forEach(function (handle) {
        var preview = self.state.preview[handle.id];
        if (preview && Math.abs(preview.x - handle.frequency) < 0.0001 &&
                Math.abs(preview.y - handle.gain) < 0.0001) {
            delete self.state.preview[handle.id];
        }
    });
    this.requestSpectrumRedraw();
};

AnalyzerControl.prototype.beginPresentation = function (
    mode, enabled, parameterRevision, viewKey
) {
    var current = this.presentation || {};
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
};

AnalyzerControl.prototype.addCurve = function (name, args, id) {
    if (!this.pendingPresentation) return;
    var presentation = this.pendingPresentation;
    var curve = {
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
};

AnalyzerControl.prototype.updateCurve = function (name, args, id) {
    if (!this.presentation) return;
    var curve = {
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
        for (var index = 0; index < this.presentation.curves.length; index += 1) {
            if (this.presentation.curves[index].id === Number(id)) {
                this.presentation.curves[index] = {
                    id: Number(id),
                    active: curve.active,
                    values: curve.values
                };
                this.requestSpectrumRedraw();
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
    this.requestSpectrumRedraw();
};

AnalyzerControl.prototype.requestSpectrumRedraw = function () {
    if (this.spectrumRedrawTimer !== null) return;
    var self = this;
    this.spectrumRedrawTimer = new Task(function () {
        self.spectrumRedrawTimer = null;
        mgraphics.redraw();
    }, this);
    this.spectrumRedrawTimer.schedule(33);
};

AnalyzerControl.prototype.addHandle = function (args) {
    if (!this.pendingPresentation) return;
    var presentation = this.pendingPresentation;
    var handle = {
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
    for (var index = 0; index < presentation.handles.length; index += 1) {
        if (presentation.handles[index].id === handle.id) {
            presentation.handles[index] = handle;
            return;
        }
    }
    presentation.handles.push(handle);
};

AnalyzerControl.prototype.paint = function () {
    this.renderer.paint(this.presentation,
        new AnalyzerLayout(mgraphics.size[0], mgraphics.size[1]), this.state);
};

AnalyzerControl.prototype.hitTest = function (x, y) {
    if (!this.presentation) return 0;
    var layout = new AnalyzerLayout(mgraphics.size[0], mgraphics.size[1]);
    var best = 0;
    var distance = 12;
    (this.presentation.handles || []).forEach(function (handle) {
        var hx = layout.left + layout.width * handle.frequency;
        var hy = layout.top + layout.height * handle.gain;
        var next = Math.sqrt(Math.pow(x - hx, 2) + Math.pow(y - hy, 2));
        if (next < distance) { distance = next; best = handle.id; }
    });
    return best;
};

AnalyzerControl.prototype.handleById = function (id) {
    var handles = this.presentation && this.presentation.handles || [];
    for (var index = 0; index < handles.length; index += 1) {
        if (handles[index].id === id) return handles[index];
    }
    return null;
};

AnalyzerControl.prototype.canMove = function (id) {
    var handle = this.handleById(id);
    var capabilities = handle && handle.capabilities;
    return Boolean(capabilities &&
        (capabilities.frequency || capabilities.gain));
};

AnalyzerControl.prototype.canChangeQ = function (id) {
    var handle = this.handleById(id);
    return Boolean(handle && handle.capabilities && handle.capabilities.q);
};

AnalyzerControl.prototype.emitIntent = function (name, values) {
    outlet(0, [name].concat(values || []));
};

AnalyzerControl.prototype.scheduleMove = function (id, x, y) {
    this.state.pendingMove = [id, x, y];
    if (this.state.moveTimer !== null) return;
    var self = this;
    this.state.moveTimer = new Task(function () {
        self.state.moveTimer = null;
        self.flushMove();
    }, this);
    this.state.moveTimer.schedule(33);
};

AnalyzerControl.prototype.flushMove = function () {
    if (!this.state.pendingMove) return;
    var move = this.state.pendingMove;
    this.state.pendingMove = null;
    this.emitIntent("filterMoved", move);
};

function presentation(value) { analyzerControl.applyPresentation(value); }
function paint() { analyzerControl.paint(); }
function onclick(x, y) {
    var id = analyzerControl.hitTest(x, y);
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
    var layout = new AnalyzerLayout(mgraphics.size[0], mgraphics.size[1]);
    var handle = analyzerControl.handleById(
        analyzerControl.state.selectedId);
    var xMinimum = isFinite(Number(handle.xMinimum))
        ? Number(handle.xMinimum) : 0;
    var xMaximum = isFinite(Number(handle.xMaximum))
        ? Number(handle.xMaximum) : 1;
    var yMinimum = isFinite(Number(handle.yMinimum))
        ? Number(handle.yMinimum) : 0;
    var yMaximum = isFinite(Number(handle.yMaximum))
        ? Number(handle.yMaximum) : 1;
    var nextX = Math.max(xMinimum, Math.min(
        xMaximum, (x - layout.left) / layout.width));
    var nextY = Math.max(yMinimum, Math.min(
        yMaximum, (y - layout.top) / layout.height));
    analyzerControl.state.preview[analyzerControl.state.selectedId] = {
        x: nextX,
        y: nextY
    };
    analyzerControl.scheduleMove(
        analyzerControl.state.selectedId,
        nextX,
        nextY
    );
    analyzerControl.requestSpectrumRedraw();
}

function onwheel(x, y, delta, mod1, shift, caps, opt, mod2) {
    var id = analyzerControl.hitTest(x, y);
    if (!id || !analyzerControl.canChangeQ(id)) return;
    analyzerControl.emitIntent("filterQChanged", [id, Number(delta) * 0.05]);
}
function onidleout() { analyzerControl.endGesture(); }

function presentation_begin(mode, enabled, parameterRevision, viewKey) {
    analyzerControl.beginPresentation(
        mode, enabled, parameterRevision, viewKey);
}

function spectrum() {
    var args = arrayfromargs(arguments);
    if (analyzerControl.pendingPresentation) {
        analyzerControl.addCurve("spectrum", args);
    } else {
        analyzerControl.updateCurve("spectrum", args);
    }
}

function reference_spectrum() {
    var args = arrayfromargs(arguments);
    if (analyzerControl.pendingPresentation) {
        analyzerControl.addCurve("reference_spectrum", args);
    } else {
        analyzerControl.updateCurve("reference_spectrum", args);
    }
}

function difference_spectrum() {
    var args = arrayfromargs(arguments);
    if (analyzerControl.pendingPresentation) {
        analyzerControl.addCurve("difference_spectrum", args);
    } else {
        analyzerControl.updateCurve("difference_spectrum", args);
    }
}

function curve() {
    var args = arrayfromargs(arguments);
    var id = args.shift();
    if (analyzerControl.pendingPresentation) {
        analyzerControl.addCurve("curve", args, id);
    } else {
        analyzerControl.updateCurve("curve", args, id);
    }
}

function combined() {
    var args = arrayfromargs(arguments);
    if (analyzerControl.pendingPresentation) {
        analyzerControl.addCurve("combined", args);
    } else {
        analyzerControl.updateCurve("combined", args);
    }
}

function all_banks() {
    var args = arrayfromargs(arguments);
    if (analyzerControl.pendingPresentation) {
        analyzerControl.addCurve("all_banks", args);
    } else {
        analyzerControl.updateCurve("all_banks", args);
    }
}

function handle() {
    analyzerControl.addHandle(arrayfromargs(arguments));
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

AnalyzerControl.prototype.endGesture = function () {
    if (!this.state.dragging) return;
    if (this.state.moveTimer !== null) {
        this.state.moveTimer.cancel();
        this.state.moveTimer = null;
    }
    this.flushMove();
    this.state.dragging = false;
    this.emitIntent("gestureEnded", [this.state.selectedId]);
};

AnalyzerControl.prototype.destroy = function () {
    if (this.spectrumRedrawTimer !== null) {
        this.spectrumRedrawTimer.cancel();
        this.spectrumRedrawTimer = null;
    }
    if (this.state.moveTimer !== null) {
        this.state.moveTimer.cancel();
        this.state.moveTimer = null;
    }
    this.state.pendingMove = null;
};

function notifydeleted() {
    analyzerControl.destroy();
}

var analyzerControl = new AnalyzerControl();
