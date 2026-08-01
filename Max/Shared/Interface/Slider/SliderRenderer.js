function SliderRenderer() {
    this.renderer = new ControlRenderer();
};

SliderRenderer.prototype.Paint = function(state, geometry, theme) {
    var vertical = state.orientation === "vertical";
    var valueX = vertical ? geometry.centerX : geometry.startX
        + (geometry.endX - geometry.startX) * state.value;
    var valueY = vertical ? geometry.startY
        + (geometry.endY - geometry.startY) * state.value : geometry.centerY;

    mgraphics.set_line_cap("round");
    mgraphics.set_line_width(theme.geometry.controlLineWidth);
    this.renderer.SetColor(theme.colors.track);
    mgraphics.new_path();
    mgraphics.move_to(geometry.startX, geometry.startY);
    mgraphics.line_to(geometry.endX, geometry.endY);
    mgraphics.stroke();

    this.renderer.SetColor(state.enabled
        ? state.valueColor || theme.colors.primaryAccent
        : theme.colors.track);
    mgraphics.new_path();
    mgraphics.move_to(geometry.startX, geometry.startY);
    mgraphics.line_to(valueX, valueY);
    mgraphics.stroke();

    var fontSize = theme.typography.minimumSize;
    this.renderer.SetFont(theme.typography.fontFamily, fontSize);
    this.renderer.SetColor(state.enabled
        ? theme.colors.text
        : theme.colors.textDisabled);
    var textSize = mgraphics.text_measure(state.label);
    var valueGap = SliderOptions.valueGap;
    if (vertical) {
        mgraphics.move_to(
            Math.max(0, geometry.centerX - textSize[0] * 0.5),
            geometry.height - geometry.padding
        );
    } else {
        mgraphics.move_to(
            Math.max(0, geometry.startX - textSize[0] - valueGap),
            geometry.centerY + textSize[1] * 0.34
        );
    }
    mgraphics.show_text(state.label);
};
