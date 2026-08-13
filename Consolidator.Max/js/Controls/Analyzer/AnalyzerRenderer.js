function AnalyzerRenderer() {}

AnalyzerRenderer.prototype.paint = function (presentation, layout, state) {
    mgraphics.set_source_rgba(0.06, 0.06, 0.07, 1);
    mgraphics.rectangle(0, 0, mgraphics.size[0], mgraphics.size[1]);
    mgraphics.fill();
    if (!presentation || !presentation.enabled) return;
    var drawSpectrum = function (spectrum, color) {
        var values = spectrum && spectrum.values;
        if (!values || values.length === 0) return;
        mgraphics.set_source_rgba.apply(mgraphics, color);
        mgraphics.set_line_width(1);
        mgraphics.new_path();
        for (var index = 0; index < values.length; index += 1) {
            var x = layout.left + layout.width * index / (values.length - 1);
            var y = layout.top + layout.height * values[index];
            if (index === 0) mgraphics.move_to(x, y);
            else mgraphics.line_to(x, y);
        }
        mgraphics.stroke();
    };
    drawSpectrum(presentation.spectrum, [0.85, 0.85, 0.9, 0.7]);
    drawSpectrum(presentation.referenceSpectrum, [0.55, 0.65, 0.8, 0.55]);
    drawSpectrum(presentation.differenceSpectrum, [0.9, 0.45, 0.35, 0.65]);
    var drawCurve = function (values, color) {
        if (!values || values.length === 0) return;
        mgraphics.set_source_rgba.apply(mgraphics, color);
        mgraphics.set_line_width(1.2);
        mgraphics.new_path();
        for (var index = 0; index < values.length; index += 1) {
            var x = layout.left + layout.width * index / (values.length - 1);
            var y = layout.top + layout.height * values[index];
            if (index === 0) mgraphics.move_to(x, y);
            else mgraphics.line_to(x, y);
        }
        mgraphics.stroke();
    };
    var combined = presentation.combinedCurve;
    drawCurve(combined && combined.values,
        combined && combined.active === false
            ? [0.35, 0.35, 0.4, 0.6] : [0.2, 0.8, 1, 1]);
    (presentation.curves || []).forEach(function (curve) {
        drawCurve(curve.values, curve.active === false
            ? [0.3, 0.3, 0.35, 0.5] : [0.45, 0.45, 0.5, 0.65]);
    });
    (presentation.handles || []).forEach(function (handle) {
        if (!handle.enabled) return;
        var preview = state && state.preview[handle.id];
        var frequency = preview ? preview.x : handle.frequency;
        var gain = preview ? preview.y : handle.gain;
        var x = layout.left + layout.width * frequency;
        var y = layout.top + layout.height * gain;
        mgraphics.set_source_rgba(handle.selected ? 1 : 0.95, 0.75, 0.25, 1);
        mgraphics.ellipse(x - 4, y - 4, 8, 8);
        mgraphics.fill();
    });
};
