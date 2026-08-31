autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { ButtonPresentation } = require("../../Presenters/Button/ButtonPresentation.js");
const { DoubleClickTracker } = require("../DoubleClickTracker.js");
const { UiColors } = require("../../Theme/UiColors.js");

const ToggleControlOptions = {
    background: UiColors.base.background,
    active: UiColors.controls.active,
    inactive: UiColors.base.lines,
    disabled: UiColors.base.disabledText,
    text: UiColors.base.activeText,
    trackWidth: 28,
    trackHeight: 14
};

class ToggleControl
{
    constructor()
    {
        this.presentation = new ButtonPresentation();
        this.presentationDepth = 0;
        this.presentationDirty = false;
        this.doubleClick = new DoubleClickTracker();
    }

    requestRedraw()
    {
        if (this.presentationDepth > 0) {
            this.presentationDirty = true;
            return;
        }
        mgraphics.redraw();
    }

    beginPresentation()
    {
        this.presentationDepth += 1;
    }

    endPresentation()
    {
        if (this.presentationDepth === 0) return;
        this.presentationDepth -= 1;
        if (this.presentationDepth === 0 && this.presentationDirty) {
            this.presentationDirty = false;
            mgraphics.redraw();
        }
    }

    applyPresentation(presentation)
    {
        if (!presentation) return;
        this.presentation = presentation;
        this.requestRedraw();
    }

    emit(name, value)
    {
        if (value === undefined) outlet(0, name);
        else outlet(0, [name, value]);
    }

    paint()
    {
        let width = mgraphics.size[0];
        let height = mgraphics.size[1];
        let presentation = this.presentation;
        let enabled = Boolean(presentation.enabled);
        let selected = Boolean(presentation.value);
        let trackWidth = Math.min(ToggleControlOptions.trackWidth, width - 4);
        let trackHeight = Math.min(ToggleControlOptions.trackHeight, height - 4);
        let trackX = width - trackWidth - 4;
        let trackY = (height - trackHeight) * 0.5;
        let trackColor = !enabled ? ToggleControlOptions.disabled :
            selected ? ToggleControlOptions.active :
            ToggleControlOptions.inactive;
        if (enabled && selected && presentation.scopeActive && presentation.scopeColor) {
            trackColor = presentation.scopeColor;
        }

        mgraphics.set_source_rgba.apply(mgraphics, ToggleControlOptions.background);
        mgraphics.rectangle(0, 0, width, height);
        mgraphics.fill();

        let knobColor = selected && enabled
            ? ToggleControlOptions.background
            : trackColor;
        mgraphics.set_source_rgba.apply(mgraphics, trackColor);
        mgraphics.rectangle(trackX, trackY, trackWidth, trackHeight);
        mgraphics.fill();
        if (!selected || !enabled) {
            mgraphics.set_source_rgba.apply(mgraphics,
                ToggleControlOptions.background);
            mgraphics.rectangle(trackX + 1, trackY + 1,
                Math.max(0, trackWidth - 2), Math.max(0, trackHeight - 2));
            mgraphics.fill();
        }

        let handleSize = Math.max(0, trackHeight - 4);
        let handleY = trackY + 2;
        let knobX = selected
            ? trackX + trackWidth - handleSize - 2
            : trackX + 2;
        mgraphics.set_source_rgba.apply(mgraphics, knobColor);
        mgraphics.rectangle(knobX, handleY, handleSize, handleSize);
        mgraphics.fill();

        if (presentation.label) {
            mgraphics.select_font_face(
                UiColors.typography.controlLabelFontFamily);
            mgraphics.set_font_size(UiColors.typography.controlLabelFontSize);
            mgraphics.set_source_rgba.apply(mgraphics,
                UiColors.base.text);
            mgraphics.move_to(4, height * 0.5 + 4);
            mgraphics.show_text(String(presentation.label));
        }
        if (presentation.scopeActive && presentation.scopeColor) {
            let markerColor = presentation.scopeColor;
            mgraphics.set_source_rgba.apply(mgraphics, markerColor);
            let handleCenterX = knobX + handleSize * 0.5;
            let handleCenterY = handleY + handleSize * 0.5;
            let markerSize = Math.min(3, handleSize);
            mgraphics.rectangle(
                handleCenterX - markerSize * 0.5,
                handleCenterY - markerSize * 0.5,
                markerSize,
                markerSize
            );
            mgraphics.fill();
        }
    }

    click()
    {
        if (!this.presentation.enabled) return;
        if (this.doubleClick.isDoubleClick("toggle")) {
            this.emit("reset");
            return;
        }
        this.emit("valueChanged", this.presentation.value ? 0 : 1);
    }
}

function presentation(value) { toggleControl.applyPresentation(value); }
function paint() { toggleControl.paint(); }
function onclick() { toggleControl.click(); }
function set(value) { toggleControl.presentation.value = Number(value) !== 0; toggleControl.requestRedraw(); }
function enabled(value) { toggleControl.presentation.enabled = Number(value) !== 0; toggleControl.requestRedraw(); }
function active(value) { toggleControl.presentation.active = Number(value) !== 0; toggleControl.requestRedraw(); }
function label(value) { toggleControl.presentation.label = String(value); toggleControl.requestRedraw(); }
function scope(active, hasColor, red, green, blue, alpha) {
    toggleControl.presentation.scopeActive = Number(active) !== 0;
    toggleControl.presentation.scopeColor = Number(hasColor) !== 0
        ? [Number(red), Number(green), Number(blue), Number(alpha)] : null;
    toggleControl.requestRedraw();
}
function presentation_begin() { toggleControl.beginPresentation(); }
function presentation_end() { toggleControl.endPresentation(); }

const toggleControl = new ToggleControl();
