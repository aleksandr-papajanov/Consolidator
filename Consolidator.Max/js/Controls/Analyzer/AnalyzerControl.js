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
    mgraphics.redraw();
};

AnalyzerControl.prototype.beginPresentation = function (
    mode, enabled, parameterRevision, viewKey
) {
    this.pendingPresentation = {
        mode: String(mode),
        enabled: Number(enabled) !== 0,
        parameterRevision: Number(parameterRevision),
        viewKey: viewKey === undefined ? "" : String(viewKey),
        spectrum: null,
        referenceSpectrum: null,
        differenceSpectrum: null,
        curves: [],
        combinedCurve: null,
        allBanksCurve: null,
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
        selected: Number(args[7]) !== 0
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
    var nextX = (x - layout.left) / layout.width;
    var nextY = (y - layout.top) / layout.height;
    analyzerControl.state.preview[analyzerControl.state.selectedId] = {
        x: nextX,
        y: nextY
    };
    analyzerControl.scheduleMove(
        analyzerControl.state.selectedId,
        nextX,
        nextY
    );
    mgraphics.redraw();
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
    analyzerControl.addCurve("spectrum", arrayfromargs(arguments));
}

function reference_spectrum() {
    analyzerControl.addCurve("reference_spectrum", arrayfromargs(arguments));
}

function difference_spectrum() {
    analyzerControl.addCurve("difference_spectrum", arrayfromargs(arguments));
}

function curve() {
    var args = arrayfromargs(arguments);
    var id = args.shift();
    analyzerControl.addCurve("curve", args, id);
}

function combined() {
    analyzerControl.addCurve("combined", arrayfromargs(arguments));
}

function all_banks() {
    analyzerControl.addCurve("all_banks", arrayfromargs(arguments));
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
