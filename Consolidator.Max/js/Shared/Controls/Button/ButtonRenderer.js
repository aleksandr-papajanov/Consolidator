const { UiColors } = require("../../Theme/UiColors.js");

const ButtonControlOptions = {
    background: UiColors.base.background,
    active: UiColors.controls.active,
    inactive: UiColors.base.lines,
    disabled: UiColors.base.disabledText,
    text: UiColors.base.activeText,
    fontSize: 12
};

class ButtonRenderer
{
    paint(presentation)
    {
        let options = ButtonControlOptions;
        let width = mgraphics.size[0];
        let height = mgraphics.size[1];
        let selected = Boolean(presentation.value);
        let color = !presentation.enabled
            ? options.disabled : selected ? options.active : options.inactive;
        mgraphics.set_source_rgba.apply(mgraphics, options.background);
        mgraphics.rectangle(0, 0, width, height);
        mgraphics.fill();
        mgraphics.set_source_rgba.apply(mgraphics, color);
        mgraphics.rectangle(1, 1, Math.max(0, width - 2), Math.max(0, height - 2));
        if (selected && presentation.enabled) mgraphics.fill();
        else mgraphics.stroke();
        this.paintLabel(presentation, width, height);
        if (presentation.scopeActive && presentation.scopeColor) {
            mgraphics.set_source_rgba.apply(mgraphics, presentation.scopeColor);
            mgraphics.arc(width - 4, 4, 2, 0, Math.PI * 2);
            mgraphics.fill();
        }
    }

    paintLabel(presentation, width, height)
    {
        if (!presentation.label) return;
        let options = ButtonControlOptions;
        mgraphics.select_font_face("Arial");
        mgraphics.set_font_size(options.fontSize);
        mgraphics.set_source_rgba.apply(mgraphics,
            presentation.enabled ? options.text : UiColors.base.disabledText);
        let textWidth = String(presentation.label).length * options.fontSize * 0.55;
        mgraphics.move_to(Math.max(2, (width - textWidth) * 0.5),
            height * 0.5 + options.fontSize * 0.35);
        mgraphics.show_text(String(presentation.label));
    }
}

module.exports = {
    ButtonRenderer: ButtonRenderer
};
