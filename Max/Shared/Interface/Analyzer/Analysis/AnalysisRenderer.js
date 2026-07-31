
function AnalysisRenderer() {
    this.controlsRenderer = new AnalyzerControlsRenderer();
}

AnalysisRenderer.prototype.Paint = function(state, width, height) {
    var settings = analysisOptions;
    this.FillBackground(settings.background, width, height);
    this.controlsRenderer.Paint(state, width);

    var bands = state.analysis.bands;
    var sectorCount = Math.max(1, bands.length + 1);
    var sectorWidth = width / sectorCount;
    var top = settings.plotTop;
    var bottom = Math.max(top + 1, height - settings.plotBottomPadding);
    var zero = (top + bottom) * 0.5;
    mgraphics.set_source_rgba(settings.grid.r, settings.grid.g, settings.grid.b, settings.grid.a);
    mgraphics.set_line_width(InterfaceTheme.geometry.borderLineWidth);
    mgraphics.move_to(0, zero);
    mgraphics.line_to(width, zero);
    mgraphics.stroke();

    for (var sector = 0; sector < sectorCount; sector++) {
        this.DrawMetricSector(
            sector < bands.length ? bands[sector].metrics : state.analysis.metrics,
            sector * sectorWidth,
            sectorWidth,
            top,
            bottom,
            zero,
            sector === bands.length ? "TOTAL" : this.BandLabel(bands[sector])
        );
    }
};

AnalysisRenderer.prototype.FillBackground = function(color, width, height) {
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();
};

AnalysisRenderer.prototype.DrawMetricSector = function(metrics, x, width, top, bottom, zero, label) {
    var settings = analysisOptions;
    var count = Math.max(1, metrics.length);
    var barWidth = Math.max(1, (width - 6) / count);
    for (var index = 0; index < metrics.length; index++) {
        var metric = metrics[index];
        var difference = Number(metric.reference) - Number(metric.current);
        if (metric.currentMean !== undefined) {
            difference = Number(metric.referenceMean) - Number(metric.currentMean);
        }
        var score = 1 - Math.exp(-Math.abs(difference) * settings.normalizationSensitivity);
        var barHeight = score * Math.min(zero - top, bottom - zero);
        var barX = x + 3 + index * barWidth;
        var color = settings.metricColors[index % settings.metricColors.length];
        mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
        mgraphics.rectangle(
            barX,
            difference >= 0 ? zero - barHeight : zero,
            Math.max(1, barWidth - 1),
            barHeight
        );
        mgraphics.fill();
    }
    mgraphics.set_source_rgba(settings.label.r, settings.label.g, settings.label.b, settings.label.a);
    mgraphics.set_font_size(settings.labelSize);
    mgraphics.move_to(x + 3, bottom + 10);
    mgraphics.show_text(label);
};

AnalysisRenderer.prototype.BandLabel = function(band) {
    if (!band) return "";
    var format = function(value) {
        return value >= 1000 ? (value / 1000) + "k" : String(value);
    };
    return format(band.minimumHz) + "-" + format(band.maximumHz);
};
