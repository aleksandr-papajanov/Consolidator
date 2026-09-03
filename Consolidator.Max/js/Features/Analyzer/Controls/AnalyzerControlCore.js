const { AnalyzerViewState } = require("./AnalyzerViewState.js");
const { AnalyzerLayout } = require("./AnalyzerLayout.js");
const { AnalyzerRenderer } = require("./AnalyzerRenderer.js");
const { AnalyzerPresentationBuilder } = require(
    "./AnalyzerPresentationBuilder.js");
const { DoubleClickTracker } = require(
    "../../../Shared/Controls/DoubleClickTracker.js");

const RedrawIntervalMs = 16;
const MoveIntervalMs = 33;

class AnalyzerControlCore
{
    constructor(taskConstructor)
    {
        this.presentation = null;
        this.pendingPresentation = null;
        this.state = new AnalyzerViewState();
        this.renderer = new AnalyzerRenderer();
        this.builder = new AnalyzerPresentationBuilder();
        this.parameterRevision = 0;
        this.viewKey = "";
        this.redrawScheduled = false;
        this.redrawTimer = new taskConstructor(() => {
            this.redrawScheduled = false;
            mgraphics.redraw();
        }, this);
        this.moveScheduled = false;
        this.moveTimer = new taskConstructor(() => {
            this.moveScheduled = false;
            this.flushMove();
        }, this);
        this.doubleClick = new DoubleClickTracker();
    }

    resetInteractionState()
    {
        if (this.moveScheduled) {
            this.moveTimer.cancel();
            this.moveScheduled = false;
        }
        this.state.preview = {};
        this.state.pendingMove = null;
        this.state.dragging = false;
        this.state.selectedId = 0;
    }

    applyPresentation(presentation)
    {
        this.presentation = presentation;
        let revision = Number(presentation.parameterRevision);
        let viewKey = presentation.viewKey === undefined
            ? "" : String(presentation.viewKey);
        if (revision !== this.parameterRevision || viewKey !== this.viewKey) {
            this.resetInteractionState();
            this.parameterRevision = revision;
            this.viewKey = viewKey;
        }
        if (!presentation.handles || presentation.handles.length === 0) {
            this.resetInteractionState();
        }
        (presentation.handles || []).forEach((handle) => {
            let preview = this.state.preview[handle.id];
            if (preview && Math.abs(preview.x - handle.frequency) < 0.0001 &&
                    Math.abs(preview.y - handle.gain) < 0.0001) {
                delete this.state.preview[handle.id];
            }
        });
        this.requestRedraw();
    }

    beginPresentation(mode, enabled, revision, viewKey)
    {
        this.pendingPresentation = this.builder.begin(
            this.presentation, mode, enabled, revision, viewKey);
    }

    addCurve(name, args, id)
    {
        this.builder.setCurve(this.pendingPresentation, name, args, id);
    }

    updateCurve(name, args, id)
    {
        if (this.builder.setCurve(this.presentation, name, args, id)) {
            this.requestRedraw();
        }
    }

    addHandle(args)
    {
        this.builder.addHandle(this.pendingPresentation, args);
    }

    setScope(active, hasColor, red, green, blue, alpha)
    {
        this.builder.setScope(this.pendingPresentation || this.presentation,
            active, hasColor, red, green, blue, alpha);
        this.requestRedraw();
    }

    requestRedraw()
    {
        if (this.redrawScheduled) return;
        this.redrawScheduled = true;
        this.redrawTimer.schedule(RedrawIntervalMs);
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
            if (next < distance) {
                distance = next;
                best = handle.id;
            }
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
        return Boolean(handle && handle.capabilities && handle.capabilities.gain);
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
        this.moveTimer.schedule(MoveIntervalMs);
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
        if (this.redrawScheduled) this.redrawTimer.cancel();
        if (this.moveScheduled) this.moveTimer.cancel();
        this.redrawScheduled = false;
        this.moveScheduled = false;
        this.state.pendingMove = null;
    }
}

module.exports = {
    AnalyzerControlCore: AnalyzerControlCore
};
