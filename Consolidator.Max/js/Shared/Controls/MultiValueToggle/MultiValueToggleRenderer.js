const { UiColors } = require("../../Theme/UiColors.js");
const {
    MultiValueToggleOptions,
    togglePoint
} = require("./MultiValueToggleGeometry.js");

class MultiValueToggleRenderer
{
    paint(control)
    {
        let options = MultiValueToggleOptions;
        let width = mgraphics.size[0];
        let height = mgraphics.size[1];
        let count = control.values.length;
        let active = control.enabled && control.active;
        let centerX = width * 0.5;
        let centerY = height * 0.55;
        let radius = Math.max(1, Math.min(width, height) * 0.38);
        let selected = count > 0
            ? Math.max(0, Math.min(count - 1, control.value)) : 0;
        let highlighted = control.hoverIndex >= 0
            ? control.hoverIndex : selected;
        mgraphics.set_source_rgba.apply(mgraphics, options.background);
        mgraphics.rectangle(0, 0, width, height);
        mgraphics.fill();
        if (count > 0) {
            this.paintOptions(control, highlighted, active,
                centerX, centerY, radius);
        }
        mgraphics.select_font_face(UiColors.typography.controlLabelFontFamily);
        mgraphics.set_font_size(UiColors.typography.controlLabelFontSize);
        mgraphics.set_source_rgba.apply(mgraphics, UiColors.base.text);
        let label = control.values[highlighted] || "";
        let textSize = mgraphics.text_measure(label);
        mgraphics.move_to((width - textSize[0]) * 0.5, height - 7);
        mgraphics.show_text(label);
    }

    paintOptions(control, highlighted, active, centerX, centerY, radius)
    {
        let options = MultiValueToggleOptions;
        let indicatorPoint = togglePoint(
            highlighted, control.values.length, centerX, centerY, radius);
        let indicatorColor = active ? options.active : options.inactive;
        if (control.scopeActive && control.scopeColor) {
            indicatorColor = control.scopeColor;
        }
        mgraphics.set_source_rgba.apply(mgraphics, indicatorColor);
        mgraphics.set_line_width(options.indicatorWidth);
        let pointerX = indicatorPoint.x - centerX;
        let pointerY = indicatorPoint.y - centerY;
        mgraphics.move_to(centerX + pointerX * options.pointerStart,
            centerY + pointerY * options.pointerStart);
        mgraphics.line_to(centerX + pointerX * options.pointerEnd,
            centerY + pointerY * options.pointerEnd);
        mgraphics.stroke();
        for (let index = 0; index < control.values.length; index += 1) {
            let point = togglePoint(
                index, control.values.length, centerX, centerY, radius);
            let highlightedOption = index === highlighted;
            mgraphics.set_source_rgba.apply(mgraphics,
                highlightedOption && active ? indicatorColor : options.ring);
            mgraphics.ellipse(point.x - 2, point.y - 2, 4, 4);
            mgraphics.fill();
        }
    }
}

module.exports = {
    MultiValueToggleRenderer: MultiValueToggleRenderer
};
