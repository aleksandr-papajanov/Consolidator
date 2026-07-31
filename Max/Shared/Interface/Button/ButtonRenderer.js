include("../Core/ControlRenderer.js");

function ButtonRenderer() {
    this.renderer = new ControlRenderer();
};

ButtonRenderer.prototype.Paint = function(state, size, options, theme) {
    this.PaintInRect(state, { x: 0, y: 0, width: size.width, height: size.height }, options, theme);
};

ButtonRenderer.prototype.PaintInRect = function(state, rect, options, theme) {
    var size = { width: rect.width, height: rect.height };
    var padding = Math.min(size.width, size.height) * options.paddingRatio;
    var radius = Math.min(size.width, size.height) * options.cornerRadiusRatio;
    var color = state.fillColor || (!state.enabled
        ? theme.states.disabled.fill
        : state.active
            ? theme.states.active.fill
            : theme.states.inactive.fill);

    this.renderer.SetColor(color);
    this.renderer.DrawRoundedRect(
        rect.x + padding,
        rect.y + padding,
        size.width - padding * 2,
        size.height - padding * 2,
        radius
    );
    mgraphics.fill();

    var borderColor = state.borderColor || (!state.enabled
        ? theme.states.disabled.border
        : state.active
            ? theme.states.active.border
            : theme.states.inactive.border);
    if (borderColor) {
        var borderWidth = theme.geometry.borderLineWidth;
        var borderInset = borderWidth * 0.5;
        this.renderer.SetColor(borderColor);
        mgraphics.set_line_width(borderWidth);
        this.renderer.DrawRoundedRect(
            rect.x + padding + borderInset,
            rect.y + padding + borderInset,
            Math.max(0, size.width - padding * 2 - borderWidth),
            Math.max(0, size.height - padding * 2 - borderWidth),
            Math.max(0, radius - borderInset)
        );
        mgraphics.stroke();
    }

    var textColor = state.textColor || (!state.enabled
        ? theme.states.disabled.text
        : state.active
            ? theme.states.active.text
            : theme.states.inactive.text);
    var fontSize = theme.typography.minimumSize;
    this.renderer.DrawCenteredText(
        state.label,
        rect,
        textColor,
        theme.typography.fontFamily,
        fontSize
    );
};
