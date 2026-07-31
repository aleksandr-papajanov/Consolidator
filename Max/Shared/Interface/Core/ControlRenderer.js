function ControlRenderer() {}

ControlRenderer.prototype.SetFont = function(fontFamily, fontSize) {
    mgraphics.select_font_face(fontFamily);
    mgraphics.set_font_size(fontSize);
};

ControlRenderer.prototype.SetColor = function(color) {
    if (!color || color.length < 4) {
        return;
    }

    mgraphics.set_source_rgba(color[0], color[1], color[2], color[3]);
};

ControlRenderer.prototype.DrawRoundedRect = function(x, y, width, height, radius) {
    mgraphics.new_path();
    mgraphics.move_to(x + radius, y);
    mgraphics.line_to(x + width - radius, y);
    mgraphics.curve_to(x + width, y, x + width, y, x + width, y + radius);
    mgraphics.line_to(x + width, y + height - radius);
    mgraphics.curve_to(
        x + width, y + height,
        x + width, y + height,
        x + width - radius, y + height
    );
    mgraphics.line_to(x + radius, y + height);
    mgraphics.curve_to(
        x, y + height,
        x, y + height,
        x, y + height - radius
    );
    mgraphics.line_to(x, y + radius);
    mgraphics.curve_to(x, y, x, y, x + radius, y);
    mgraphics.close_path();
};

ControlRenderer.prototype.DrawCenteredText = function(text, rect, color, fontFamily, fontSize) {
    this.SetFont(fontFamily, fontSize);
    this.SetColor(color);
    var textSize = mgraphics.text_measure(text);
    var fontExtents = mgraphics.font_extents();
    var ascent = Number(fontExtents[0]);
    var descent = Number(fontExtents[1]);
    mgraphics.move_to(
        rect.x + (rect.width - textSize[0]) * 0.5,
        rect.y + (rect.height + ascent - descent) * 0.5
    );
    mgraphics.show_text(text);
};
