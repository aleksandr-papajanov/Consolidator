autowatch = 1;
inlets = 1;
outlets = 1;
include("AnalyzerViewState.js");
include("AnalyzerLayout.js");
include("AnalyzerRenderer.js");

function AnalyzerControl() {
    this.presentation = null;
    this.state = new AnalyzerViewState();
    this.renderer = new AnalyzerRenderer();
}

AnalyzerControl.prototype.applyPresentation = function (presentation) {
    this.presentation = presentation;
    var self = this;
    (presentation.handles || []).forEach(function (handle) {
        var preview = self.state.preview[handle.id];
        if (preview && Math.abs(preview.x - handle.frequency) < 0.0001 &&
                Math.abs(preview.y - handle.gain) < 0.0001) {
            delete self.state.preview[handle.id];
        }
    });
    mgraphics.redraw();
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
    this.state.moveTimer = setTimeout(function () {
        self.state.moveTimer = null;
        self.flushMove();
    }, 33);
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

AnalyzerControl.prototype.endGesture = function () {
    if (!this.state.dragging) return;
    if (this.state.moveTimer !== null) {
        clearTimeout(this.state.moveTimer);
        this.state.moveTimer = null;
    }
    this.flushMove();
    this.state.dragging = false;
    this.emitIntent("gestureEnded", [this.state.selectedId]);
};

var analyzerControl = new AnalyzerControl();
