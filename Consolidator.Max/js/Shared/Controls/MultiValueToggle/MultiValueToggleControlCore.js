const {
    MultiValueToggleOptions,
    togglePoint
} = require("./MultiValueToggleGeometry.js");
const { MultiValueToggleRenderer } = require("./MultiValueToggleRenderer.js");

class MultiValueToggleControlCore
{
    constructor()
    {
        this.value = 0;
        this.values = [];
        this.enabled = true;
        this.active = true;
        this.scopeActive = false;
        this.scopeColor = null;
        this.inPresentation = false;
        this.hoverIndex = -1;
        this.dragging = false;
        this.lastY = 0;
        this.dragValue = 0;
        this.renderer = new MultiValueToggleRenderer();
    }

    redraw() { if (!this.inPresentation) mgraphics.redraw(); }

    beginGesture(y)
    {
        if (!this.enabled || !this.active || this.values.length === 0) return;
        this.dragging = true;
        this.lastY = y;
        this.dragValue = this.value;
    }

    drag(y)
    {
        if (!this.dragging || this.values.length < 2) return;
        let stepCount = this.values.length - 1;
        let nextValue = this.dragValue + (this.lastY - y) *
            MultiValueToggleOptions.dragSensitivity * stepCount;
        let next = Math.max(0, Math.min(stepCount, Math.round(nextValue)));
        this.dragValue = nextValue;
        this.lastY = y;
        if (next === this.value) return;
        this.value = next;
        this.hoverIndex = next;
        this.redraw();
        outlet(0, ["valueChanged", next]);
    }

    endGesture() { this.dragging = false; }

    point(index, count, centerX, centerY, radius) {
        return togglePoint(index, count, centerX, centerY, radius);
    }

    updateHover(x, y)
    {
        if (this.dragging || this.values.length === 0) return;
        let width = mgraphics.size[0];
        let height = mgraphics.size[1];
        let centerX = width * 0.5;
        let centerY = height * 0.55;
        let radius = Math.max(1, Math.min(width, height) * 0.38);
        let nearest = -1;
        let nearestDistance = Infinity;
        for (let index = 0; index < this.values.length; index += 1) {
            let point = togglePoint(index, this.values.length,
                centerX, centerY, radius);
            let distance = Math.sqrt(
                Math.pow(x - point.x, 2) + Math.pow(y - point.y, 2));
            if (distance < nearestDistance) {
                nearest = index;
                nearestDistance = distance;
            }
        }
        let next = nearestDistance <= Math.max(10, width * 0.16) ? nearest : -1;
        if (next === this.hoverIndex) return;
        this.hoverIndex = next;
        this.redraw();
    }

    clearHover()
    {
        if (this.hoverIndex === -1) return;
        this.hoverIndex = -1;
        this.redraw();
    }

    paint() { this.renderer.paint(this); }
}

module.exports = {
    MultiValueToggleControlCore: MultiValueToggleControlCore
};
