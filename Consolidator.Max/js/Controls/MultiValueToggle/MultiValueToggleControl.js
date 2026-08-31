autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { UiColors } = require("../../Theme/UiColors.js");

const MultiValueToggleOptions = {
    startAngle: Math.PI * (5 / 6),
    angleRange: Math.PI * (4 / 3),
    pointAngle: Math.PI / 3,
    lineWidth: 3,
    indicatorWidth: 2,
    pointerStart: 0.2,
    pointerEnd: 0.6,
    dragSensitivity: 0.015,
    background: UiColors.base.background,
    ring: UiColors.controls.ring,
    active: UiColors.base.brightText,
    inactive: UiColors.controls.inactive
};

class MultiValueToggleControl
{
    constructor()
    {
        this.value = 0;
        this.values = [];
        this.enabled = true;
        this.active = true;
        this.inPresentation = false;
        this.hoverIndex = -1;
        this.dragging = false;
        this.lastY = 0;
        this.dragValue = 0;
    }

    redraw()
    {
        if (!this.inPresentation) mgraphics.redraw();
    }

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

    endGesture()
    {
        this.dragging = false;
    }

    point(index, count, centerX, centerY, radius)
    {
        let span = Math.min(MultiValueToggleOptions.angleRange,
            MultiValueToggleOptions.pointAngle * Math.max(0, count - 1));
        let start = MultiValueToggleOptions.startAngle +
            (MultiValueToggleOptions.angleRange - span) * 0.5;
        let angle = start + (count <= 1 ? span * 0.5 :
            span * index / (count - 1));
        return {
            x: centerX + Math.cos(angle) * radius,
            y: centerY + Math.sin(angle) * radius
        };
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
            let point = this.point(index, this.values.length,
                centerX, centerY, radius);
            let distance = Math.sqrt(Math.pow(x - point.x, 2) +
                Math.pow(y - point.y, 2));
            if (distance < nearestDistance) {
                nearest = index;
                nearestDistance = distance;
            }
        }
        let next = nearestDistance <= Math.max(10, width * 0.16)
            ? nearest : -1;
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

    paint()
    {
        let width = mgraphics.size[0];
        let height = mgraphics.size[1];
        let count = this.values.length;
        let active = this.enabled && this.active;
        let centerX = width * 0.5;
        let centerY = height * 0.55;
        let radius = Math.max(1, Math.min(width, height) * 0.38);
        let selected = count > 0
            ? Math.max(0, Math.min(count - 1, this.value)) : 0;
        let highlighted = this.hoverIndex >= 0 ? this.hoverIndex : selected;

        mgraphics.set_source_rgba.apply(mgraphics,
            MultiValueToggleOptions.background);
        mgraphics.rectangle(0, 0, width, height);
        mgraphics.fill();

        if (count > 0) {
            let indicatorPoint = this.point(highlighted, count,
                centerX, centerY, radius);
            mgraphics.set_source_rgba.apply(mgraphics,
                active ? MultiValueToggleOptions.active :
                    MultiValueToggleOptions.inactive);
            mgraphics.set_line_width(MultiValueToggleOptions.indicatorWidth);
            let pointerX = indicatorPoint.x - centerX;
            let pointerY = indicatorPoint.y - centerY;
            mgraphics.move_to(
                centerX + pointerX * MultiValueToggleOptions.pointerStart,
                centerY + pointerY * MultiValueToggleOptions.pointerStart
            );
            mgraphics.line_to(
                centerX + pointerX * MultiValueToggleOptions.pointerEnd,
                centerY + pointerY * MultiValueToggleOptions.pointerEnd
            );
            mgraphics.stroke();

            for (let index = 0; index < count; index += 1) {
                let point = this.point(index, count, centerX, centerY, radius);
                let isHighlighted = index === highlighted;
                let dotRadius = 2;
                mgraphics.set_source_rgba.apply(mgraphics,
                    isHighlighted && active
                        ? MultiValueToggleOptions.active :
                        MultiValueToggleOptions.ring);
                mgraphics.ellipse(point.x - dotRadius, point.y - dotRadius,
                    dotRadius * 2, dotRadius * 2);
                mgraphics.fill();
            }
        }

        mgraphics.select_font_face(
            UiColors.typography.controlLabelFontFamily);
        mgraphics.set_font_size(UiColors.typography.controlLabelFontSize);
        mgraphics.set_source_rgba.apply(mgraphics,
            UiColors.base.inactiveText);
        let label = this.values[highlighted] || "";
        let textSize = mgraphics.text_measure(label);
        let fontExtents = mgraphics.font_extents();
        let labelX = (width - textSize[0]) * 0.5;
        let labelY = height - 7;
        mgraphics.move_to(labelX, labelY);
        mgraphics.show_text(label);
    }
}

function presentation(value) { multiValueToggleControl.value = Number(value.value); multiValueToggleControl.values = value.values || []; multiValueToggleControl.redraw(); }
function presentation_begin() { multiValueToggleControl.inPresentation = true; }
function presentation_end() { multiValueToggleControl.inPresentation = false; mgraphics.redraw(); }
function paint() { multiValueToggleControl.paint(); }
function onclick(x, y) {
    multiValueToggleControl.updateHover(x, y);
    multiValueToggleControl.beginGesture(y);
}
function ondrag(x, y, button) {
    if (button === 0) multiValueToggleControl.endGesture();
    else multiValueToggleControl.drag(y);
}
function onmousemove(x, y) { multiValueToggleControl.updateHover(x, y); }
function onidleout() {
    multiValueToggleControl.endGesture();
    multiValueToggleControl.clearHover();
}
function set(value) { multiValueToggleControl.value = Number(value); multiValueToggleControl.redraw(); }
function values() { multiValueToggleControl.values = arrayfromargs(arguments).map(String); multiValueToggleControl.redraw(); }
function enabled(value) { multiValueToggleControl.enabled = Number(value) !== 0; multiValueToggleControl.redraw(); }
function active(value) { multiValueToggleControl.active = Number(value) !== 0; multiValueToggleControl.redraw(); }

const multiValueToggleControl = new MultiValueToggleControl();
