function AnalyzerViewRenderer() {
}

AnalyzerViewRenderer.prototype.Paint = function(state) {
    var size = mgraphics.size;
    if (state.mode === "analysis") this.PaintAnalysis(state, size[0], size[1]);
    else this.PaintSpectrum(state, size[0], size[1]);
};

AnalyzerViewRenderer.prototype.PaintSpectrum = function(state, width, height) {
    var settings = analyzerViewConfig.spectrum;
    var bottom = analyzerViewGeometry.PlotBottom(height);
    this.FillBackground(settings.background, width, height);
    this.DrawViewControls(state, width);
    this.DrawSpectrumGrid(width, bottom);
    this.DrawCurve(state.totalCurve, width, bottom, settings.total, settings.totalLineWidth);
    for (var filterId in state.filterCurves) {
        if (state.filterCurves.hasOwnProperty(filterId)) {
            var filterCurve = state.filterCurves[filterId];
            if (filterCurve.type !== "gain") {
                this.DrawCurve(filterCurve.curve, width, bottom,
                    filterCurve.color, settings.filterLineWidth);
            }
        }
    }
    this.DrawCurve(state.fitCurve, width, bottom, settings.fit, settings.currentLineWidth);
    this.DrawCurve(state.referenceCurve, width, bottom, settings.reference, settings.currentLineWidth);
    this.DrawCurve(state.currentCurve, width, bottom, settings.current, settings.currentLineWidth);
    this.DrawHandles(state, width, bottom);
    this.DrawFrequencyLabels(width, height);
};

AnalyzerViewRenderer.prototype.FillBackground = function(color, width, height) {
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();
};

AnalyzerViewRenderer.prototype.DrawSpectrumGrid = function(width, bottom) {
    var settings = analyzerViewConfig.spectrum;
    mgraphics.set_line_width(1);
    mgraphics.set_source_rgba(settings.grid.r, settings.grid.g, settings.grid.b, settings.grid.a);
    for (var db = -settings.scaleDb; db <= settings.scaleDb; db += settings.dbStep) {
        var y = analyzerViewGeometry.DbToY(db, bottom);
        mgraphics.move_to(0, y);
        mgraphics.line_to(width, y);
        mgraphics.stroke();
    }
    for (var index = 0; index < settings.minorFrequencies.length; ++index) {
        var x = analyzerViewGeometry.FrequencyToX(settings.minorFrequencies[index], width);
        mgraphics.move_to(x, analyzerViewGeometry.PlotTop());
        mgraphics.line_to(x, bottom);
        mgraphics.stroke();
    }
    mgraphics.set_source_rgba(settings.majorGrid.r, settings.majorGrid.g, settings.majorGrid.b, settings.majorGrid.a);
    mgraphics.set_line_width(1.5);
    for (var major = 0; major < settings.majorFrequencies.length; ++major) {
        var majorX = analyzerViewGeometry.FrequencyToX(settings.majorFrequencies[major], width);
        mgraphics.move_to(majorX, analyzerViewGeometry.PlotTop());
        mgraphics.line_to(majorX, bottom);
        mgraphics.stroke();
    }
};

AnalyzerViewRenderer.prototype.DrawViewControls = function(state, width) {
    var settings = analyzerViewConfig.spectrum;
    var top = settings.controlPadding;
    var height = settings.controlHeight - settings.controlPadding * 2;
    this.DrawControl("FFT", top, 48, height, state.mode === "spectrum");
    this.DrawControl("ANALYSIS", top + 50, 64, height, state.mode === "analysis");
    this.DrawControl(settings.scaleDb + " dB", width - 46, 42, height, true);
};

AnalyzerViewRenderer.prototype.DrawControl = function(label, x, width, height, active) {
    var settings = analyzerViewConfig.spectrum;
    var color = active ? settings.controlActive : settings.controlInactive;
    if (active) {
        mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
        mgraphics.rectangle(x, settings.controlPadding, width, height);
        mgraphics.fill();
    }
    mgraphics.set_source_rgba(color.r, color.g, color.b, active ? color.a : 0.8);
    mgraphics.set_line_width(settings.controlLineWidth);
    mgraphics.rectangle(x + 0.5, settings.controlPadding + 0.5, width - 1, height - 1);
    mgraphics.stroke();
    mgraphics.set_source_rgba(
        active ? settings.background.r : color.r,
        active ? settings.background.g : color.g,
        active ? settings.background.b : color.b,
        1
    );
    mgraphics.set_font_size(7);
    mgraphics.move_to(x + 4, settings.controlPadding + height - 4);
    mgraphics.show_text(label);
};

AnalyzerViewRenderer.prototype.DrawCurve = function(values, width, bottom, color, lineWidth) {
    if (!values || values.length < 2) return;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_line_width(lineWidth);
    mgraphics.new_path();
    for (var index = 1; index < values.length; ++index) {
        this.DrawVisibleCurveSegment(
            values[index - 1], values[index],
            analyzerViewGeometry.BinToX(index - 1, values.length, width),
            analyzerViewGeometry.BinToX(index, values.length, width),
            bottom
        );
    }
    mgraphics.stroke();
};

AnalyzerViewRenderer.prototype.DrawVisibleCurveSegment = function(startDb, endDb, startX, endX, bottom) {
    var scaleDb = analyzerViewConfig.spectrum.scaleDb;
    var minimumDb = -scaleDb;
    var maximumDb = scaleDb;
    var deltaDb = endDb - startDb;
    if (deltaDb === 0) {
        if (startDb < minimumDb || startDb > maximumDb) return;
        mgraphics.move_to(startX, analyzerViewGeometry.DbToY(startDb, bottom));
        mgraphics.line_to(endX, analyzerViewGeometry.DbToY(endDb, bottom));
        return;
    }

    var startT = 0;
    var endT = 1;
    if (startDb < minimumDb) {
        if (endDb <= minimumDb) return;
        startT = (minimumDb - startDb) / deltaDb;
    } else if (startDb > maximumDb) {
        if (endDb >= maximumDb) return;
        startT = (maximumDb - startDb) / deltaDb;
    }
    if (endDb < minimumDb) {
        if (startDb <= minimumDb) return;
        endT = (minimumDb - startDb) / deltaDb;
    } else if (endDb > maximumDb) {
        if (startDb >= maximumDb) return;
        endT = (maximumDb - startDb) / deltaDb;
    }
    if (startT > endT) return;

    var clippedStartDb = startDb + deltaDb * startT;
    var clippedEndDb = startDb + deltaDb * endT;
    var deltaX = endX - startX;
    mgraphics.move_to(startX + deltaX * startT, analyzerViewGeometry.DbToY(clippedStartDb, bottom));
    mgraphics.line_to(startX + deltaX * endT, analyzerViewGeometry.DbToY(clippedEndDb, bottom));
};

AnalyzerViewRenderer.prototype.DrawHandles = function(state, width, bottom) {
    var radius = analyzerViewConfig.spectrum.handleRadius;
    for (var index = 0; index < state.handles.length; ++index) {
        var handle = state.handles[index];
        if (!handle.active) continue;
        var x = handle.type === "gain"
            ? analyzerViewConfig.spectrum.gainHandleX
            : analyzerViewGeometry.FrequencyToX(handle.frequency, width);
        var y = analyzerViewGeometry.DbToY(handle.gain, bottom);
        var color = state.filterCurves[handle.filterId]
            ? state.filterCurves[handle.filterId].color
            : analyzerViewConfig.spectrum.handle;
        mgraphics.set_source_rgba(color.r, color.g, color.b, 1);
        if (handle.type === "gain") {
            mgraphics.new_path();
            mgraphics.move_to(x + radius, y - radius);
            mgraphics.line_to(x + radius, y + radius);
            mgraphics.line_to(x - radius, y);
            mgraphics.close_path();
            mgraphics.fill();
        }
        else {
            mgraphics.ellipse(x - radius, y - radius, radius * 2, radius * 2);
            mgraphics.fill();
        }
    }
};

AnalyzerViewRenderer.prototype.DrawFrequencyLabels = function(width, height) {
    var settings = analyzerViewConfig.spectrum;
    mgraphics.set_source_rgba(settings.label.r, settings.label.g, settings.label.b, settings.label.a);
    mgraphics.set_font_size(settings.labelSize);
    mgraphics.select_font_face("Arial");
    for (var index = 0; index < settings.majorFrequencies.length; ++index) {
        var frequency = settings.majorFrequencies[index];
        var text = frequency >= 1000 ? (frequency / 1000) + "k" : String(frequency);
        var x = analyzerViewGeometry.FrequencyToX(frequency, width);
        mgraphics.move_to(x - 8, height - 4);
        mgraphics.show_text(text);
    }
};

AnalyzerViewRenderer.prototype.PaintAnalysis = function(state, width, height) {
    var settings = analyzerViewConfig.analysis;
    this.FillBackground(settings.background, width, height);
    this.DrawViewControls(state, width);
    var bands = state.analysis.bands;
    var sectorCount = Math.max(1, bands.length + 1);
    var sectorWidth = width / sectorCount;
    var top = analyzerViewGeometry.PlotTop();
    var bottom = Math.max(top + 1, height - 14);
    var zero = (top + bottom) * 0.5;
    mgraphics.set_source_rgba(settings.grid.r, settings.grid.g, settings.grid.b, settings.grid.a);
    mgraphics.set_line_width(1);
    mgraphics.move_to(0, zero);
    mgraphics.line_to(width, zero);
    mgraphics.stroke();
    for (var sector = 0; sector < sectorCount; ++sector) {
        this.DrawMetricSector(sector < bands.length ? bands[sector].metrics : state.analysis.metrics,
            sector * sectorWidth, sectorWidth, top, bottom, zero, sector === bands.length ? "TOTAL" : this.BandLabel(bands[sector]));
    }
};

AnalyzerViewRenderer.prototype.DrawMetricSector = function(metrics, x, width, top, bottom, zero, label) {
    var settings = analyzerViewConfig.analysis;
    var count = Math.max(1, metrics.length);
    var barWidth = Math.max(1, (width - 6) / count);
    for (var index = 0; index < metrics.length; ++index) {
        var metric = metrics[index];
        var difference = Number(metric.reference) - Number(metric.current);
        if (metric.currentMean !== undefined) difference = Number(metric.referenceMean) - Number(metric.currentMean);
        var score = 1 - Math.exp(-Math.abs(difference) * settings.normalizationSensitivity);
        var barHeight = score * Math.min(zero - top, bottom - zero);
        var barX = x + 3 + index * barWidth;
        var color = settings.metricColors[index % settings.metricColors.length];
        mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
        mgraphics.rectangle(barX, difference >= 0 ? zero - barHeight : zero, Math.max(1, barWidth - 1), barHeight);
        mgraphics.fill();
    }
    mgraphics.set_source_rgba(settings.label.r, settings.label.g, settings.label.b, settings.label.a);
    mgraphics.set_font_size(settings.labelSize);
    mgraphics.move_to(x + 3, bottom + 10);
    mgraphics.show_text(label);
};

AnalyzerViewRenderer.prototype.BandLabel = function(band) {
    if (!band) return "";
    function Format(value) { return value >= 1000 ? (value / 1000) + "k" : String(value); }
    return Format(band.minimumHz) + "-" + Format(band.maximumHz);
};

var analyzerViewRenderer = new AnalyzerViewRenderer();
