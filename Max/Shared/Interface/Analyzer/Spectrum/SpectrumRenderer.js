include("../../Curve/CurveRenderer.js");

function SpectrumRenderer() {
    this.curveRenderer = new CurveRenderer();
    this.controlsRenderer = new AnalyzerControlsRenderer();
}

SpectrumRenderer.prototype.Paint = function(state) {
    var size = mgraphics.size;
    this.PaintSpectrum(state, size[0], size[1]);
};

SpectrumRenderer.prototype.PaintSpectrum = function(state, width, height) {
    var settings = spectrumOptions;
    var bottom = spectrumGeometry.PlotBottom(height);
    this.FillBackground(settings.background, width, height);
    this.controlsRenderer.Paint(state, width, height);
    this.DrawSignalCurve(state.referenceCurve, width, bottom,
        settings.reference, settings.currentLineWidth);
    this.DrawSignalCurve(state.currentCurve, width, bottom,
        settings.current, settings.currentLineWidth);
    this.DrawSpectrumGrid(width, bottom);
    this.DrawCurve(state.totalCurve, width, bottom, settings.total, settings.totalLineWidth);
    for (var filterId in state.filterCurves) {
        if (state.filterCurves.hasOwnProperty(filterId)) {
            var filterCurve = state.filterCurves[filterId];
            if (filterCurve.active && filterCurve.type !== "gain") {
                this.DrawCurve(filterCurve.curve, width, bottom,
                    filterCurve.color, settings.filterLineWidth);
            }
        }
    }
    this.DrawCurve(state.fitCurve, width, bottom, settings.fit, settings.currentLineWidth);
    this.DrawHandles(state, width, bottom);
    this.DrawFrequencyLabels(width, height);
};

SpectrumRenderer.prototype.FillBackground = function(color, width, height) {
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();
};

SpectrumRenderer.prototype.DrawSpectrumGrid = function(width, bottom) {
    var settings = spectrumOptions;
    mgraphics.set_line_width(InterfaceTheme.geometry.borderLineWidth);
    mgraphics.set_source_rgba(settings.grid.r, settings.grid.g, settings.grid.b, settings.grid.a);
    this.DrawEqGrid(width, bottom);
    for (var index = 0; index < settings.minorFrequencies.length; ++index) {
        var x = spectrumGeometry.FrequencyToX(settings.minorFrequencies[index], width);
        mgraphics.move_to(x, spectrumGeometry.PlotTop());
        mgraphics.line_to(x, bottom);
        mgraphics.stroke();
    }
    mgraphics.set_source_rgba(settings.majorGrid.r, settings.majorGrid.g, settings.majorGrid.b, settings.majorGrid.a);
    mgraphics.set_line_width(InterfaceTheme.geometry.indicatorLineWidth);
    for (var major = 0; major < settings.majorFrequencies.length; ++major) {
        var majorX = spectrumGeometry.FrequencyToX(settings.majorFrequencies[major], width);
        mgraphics.move_to(majorX, spectrumGeometry.PlotTop());
        mgraphics.line_to(majorX, bottom);
        mgraphics.stroke();
    }
};

SpectrumRenderer.prototype.DrawEqGrid = function(width, bottom) {
    var settings = spectrumOptions;
    for (var db = -settings.scaleDb; db <= settings.scaleDb; db += settings.dbStep) {
        var y = spectrumGeometry.DbToY(db, bottom);
        mgraphics.move_to(0, y);
        mgraphics.line_to(width, y);
        mgraphics.stroke();
    }
};

SpectrumRenderer.prototype.DrawCurve = function(values, width, bottom, color, lineWidth) {
    if (!values || values.length < 2) return;
    var scaleDb = spectrumOptions.scaleDb;
    if (!isFinite(scaleDb) || scaleDb <= 0) return;
    this.curveRenderer.Paint(
        values,
        width,
        bottom,
        color,
        lineWidth,
        spectrumGeometry,
        -scaleDb,
        scaleDb
    );
};

SpectrumRenderer.prototype.SignalDbToY = function(value, bottom) {
    var settings = spectrumOptions;
    var range = Math.max(1, settings.signalMaximumDb - settings.signalMinimumDb);
    var clamped = Math.max(settings.signalMinimumDb,
        Math.min(settings.signalMaximumDb, value));
    var ratio = (settings.signalMaximumDb - clamped) / range;
    return spectrumGeometry.PlotTop()
        + ratio * (bottom - spectrumGeometry.PlotTop());
};

SpectrumRenderer.prototype.DrawSignalCurve = function(values, width, bottom, color, lineWidth) {
    if (!values || values.length < 2 ||
        !isFinite(spectrumOptions.signalMinimumDb) ||
        !isFinite(spectrumOptions.signalMaximumDb) ||
        spectrumOptions.signalMaximumDb <= spectrumOptions.signalMinimumDb) return;
    var renderer = this;
    var geometry = {
        BinToX: function(index, count, targetWidth) {
            return spectrumGeometry.BinToX(index, count, targetWidth);
        },
        DbToY: function(value, targetBottom) {
            return renderer.SignalDbToY(value, targetBottom);
        }
    };
    this.curveRenderer.Paint(
        values,
        width,
        bottom,
        color,
        lineWidth,
        geometry,
        spectrumOptions.signalMinimumDb,
        spectrumOptions.signalMaximumDb
    );
};

SpectrumRenderer.prototype.DrawHandles = function(state, width, bottom) {
    var radius = spectrumOptions.handleRadius;
    for (var index = 0; index < state.handles.length; ++index) {
        var handle = state.handles[index];
        var x = handle.type === "gain"
            ? spectrumOptions.gainHandleX
            : spectrumGeometry.FrequencyToX(handle.frequency, width);
        var y = spectrumGeometry.DbToY(handle.gain, bottom);
        var filterVisual = state.filterCurves[handle.filterId];
        var color = filterVisual
            ? filterVisual.color
            : spectrumOptions.handle;
        mgraphics.set_line_width(spectrumOptions.handleLineWidth);
        var opacity = filterVisual && filterVisual.neutral
            ? spectrumOptions.neutralHandleOpacity
            : 1;
        if (!handle.active) opacity *= spectrumOptions.inactiveHandleOpacity;
        mgraphics.set_source_rgba(color.r, color.g, color.b, opacity);
        var isFilled = handle.active;
        if (handle.type === "gain") {
            mgraphics.new_path();
            mgraphics.move_to(x + radius, y - radius);
            mgraphics.line_to(x + radius, y + radius);
            mgraphics.line_to(x - radius, y);
            mgraphics.close_path();
            if (isFilled) mgraphics.fill();
            else mgraphics.stroke();
        }
        else {
            mgraphics.ellipse(x - radius, y - radius, radius * 2, radius * 2);
            if (isFilled) mgraphics.fill();
            else mgraphics.stroke();
        }
    }
};

SpectrumRenderer.prototype.DrawFrequencyLabels = function(width, height) {
    var settings = spectrumOptions;
    mgraphics.set_source_rgba(settings.label.r, settings.label.g, settings.label.b, settings.label.a);
    mgraphics.select_font_face(settings.fontFamily);
    mgraphics.set_font_size(settings.labelSize);
    mgraphics.select_font_face(settings.fontFamily);
    for (var index = 0; index < settings.majorFrequencies.length; ++index) {
        var frequency = settings.majorFrequencies[index];
        var text = frequency >= 1000 ? (frequency / 1000) + "k" : String(frequency);
        var x = spectrumGeometry.FrequencyToX(frequency, width);
        mgraphics.move_to(x - 8,
            height - spectrumOptions.controlHeight - 4);
        mgraphics.show_text(text);
    }
};
