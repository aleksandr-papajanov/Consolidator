SpectrumViewController.prototype.FeatureVector = function() {
    var values = arrayfromargs(arguments);
    if (values.length < 3) return;
    var position = 0;
    var windowCount = Number(values[position++]);
    var historySeconds = Number(values[position++]);
    var metricCount = Number(values[position++]);
    if (!isFinite(windowCount) || !isFinite(historySeconds) || metricCount < 0) return;

    var metrics = [];
    for (var index = 0; index < metricCount; index++) {
        if (position + 4 >= values.length) return;
        metrics.push({
            id: String(values[position++]),
            currentMean: Number(values[position++]),
            currentDeviation: Number(values[position++]),
            referenceMean: Number(values[position++]),
            referenceDeviation: Number(values[position++])
        });
    }
    if (position + 1 >= values.length) return;
    var bandCount = Number(values[position++]);
    var bandMetricCount = Number(values[position++]);
    var bandMetricIds = [];
    for (var metricIndex = 0; metricIndex < bandMetricCount; metricIndex++) {
        if (position >= values.length) return;
        bandMetricIds.push(String(values[position++]));
    }
    var bands = [];
    for (var bandIndex = 0; bandIndex < bandCount; bandIndex++) {
        if (position + 1 >= values.length) return;
        var band = {
            minimumHz: Number(values[position++]),
            maximumHz: Number(values[position++]),
            metrics: []
        };
        for (metricIndex = 0; metricIndex < bandMetricCount; metricIndex++) {
            if (position + 3 >= values.length) return;
            band.metrics.push({
                id: bandMetricIds[metricIndex],
                current: Number(values[position++]),
                currentDeviation: Number(values[position++]),
                reference: Number(values[position++]),
                referenceDeviation: Number(values[position++])
            });
        }
        bands.push(band);
    }
    spectrumState.analysis.windowCount = windowCount;
    spectrumState.analysis.historySeconds = historySeconds;
    spectrumState.analysis.metrics = metrics;
    spectrumState.analysis.bands = bands;
    mgraphics.redraw();
};

SpectrumViewController.prototype.DrawMetricsPage = function(width, height) {
    var bands = spectrumState.analysis.bands;
    var settings = spectrumState.visualSettings.metrics;
    var top = Math.min(5, Math.max(0, height * 0.05));
    var showTable = width >= 320 && height >= 96;
    var bottom = Math.max(top + 1, height - (showTable ? 55 : 18));
    var zero = (top + bottom) * 0.5;
    var sectorCount = Math.max(1, bands.length + 1);
    var sectorWidth = width / sectorCount;

    this.DrawMetricsGrid(width, top, bottom, zero, sectorCount, sectorWidth, settings);
    for (var bandIndex = 0; bandIndex < bands.length; bandIndex++) {
        this.DrawBandMetrics(bands[bandIndex], bandIndex * sectorWidth, sectorWidth, top, bottom, zero, settings);
    }
    this.DrawOverallVector(bands.length * sectorWidth, sectorWidth, top, bottom, zero, settings);
    if (showTable) this.DrawGlobalMetricTable(width, height, settings);
};

SpectrumViewController.prototype.DrawMetricsGrid = function(width, top, bottom, zero, sectorCount, sectorWidth, settings) {
    mgraphics.set_source_rgba(settings.grid.r, settings.grid.g, settings.grid.b, settings.grid.a);
    mgraphics.set_line_width(1);
    mgraphics.move_to(0, zero);
    mgraphics.line_to(width, zero);
    mgraphics.stroke();
    for (var sector = 1; sector < sectorCount; sector++) {
        var x = sector * sectorWidth;
        mgraphics.move_to(x, top);
        mgraphics.line_to(x, bottom + 12);
        mgraphics.stroke();
    }
};

SpectrumViewController.prototype.DrawBandMetrics = function(band, x, width, top, bottom, zero, settings) {
    var barCount = band.metrics.length + 1;
    var gap = width < 80 ? 1 : 2;
    var padding = width < 80 ? 2 : 4;
    var innerWidth = width - padding * 2;
    var barWidth = Math.max(1, (innerWidth - gap * (barCount - 1)) / barCount);
    var scores = [];
    for (var index = 0; index < band.metrics.length; index++) {
        var metric = band.metrics[index];
        var difference = this.BuildMetricDifference(
            metric.id,
            metric.current,
            metric.currentDeviation,
            metric.reference,
            metric.referenceDeviation,
            band);
        scores.push(difference.score);
        this.DrawMetricBar(
            x + padding + index * (barWidth + gap), barWidth, zero, top, bottom,
            difference.direction * difference.score,
            settings.metricColors[index % settings.metricColors.length]);
    }
    var composite = this.VectorMagnitude(scores);
    this.DrawMetricBar(
        x + padding + (barCount - 1) * (barWidth + gap), barWidth, zero, top, bottom,
        composite, settings.composite);

    mgraphics.set_source_rgba(settings.label.r, settings.label.g, settings.label.b, settings.label.a);
    mgraphics.set_font_size(7);
    mgraphics.select_font_face("Arial");
    var label = this.FormatBandLabel(band.minimumHz, band.maximumHz);
    mgraphics.move_to(x + (width - this.EstimateLabelWidth(label)) * 0.5, bottom + 10);
    mgraphics.show_text(label);
};

SpectrumViewController.prototype.DrawMetricBar = function(x, width, zero, top, bottom, value, color) {
    var maximumHeight = Math.min(zero - top, bottom - zero);
    var height = Math.abs(value) * maximumHeight;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.rectangle(x, value >= 0 ? zero - height : zero, width, height);
    mgraphics.fill();
};

SpectrumViewController.prototype.DrawOverallVector = function(x, width, top, bottom, zero, settings) {
    var scores = [];
    for (var index = 0; index < spectrumState.analysis.metrics.length; index++) {
        var metric = spectrumState.analysis.metrics[index];
        scores.push(this.BuildMetricDifference(
            metric.id, metric.currentMean, metric.currentDeviation,
            metric.referenceMean, metric.referenceDeviation,
            { minimumHz: spectrumState.displayMinFrequency, maximumHz: spectrumState.displayMaxFrequency }).score);
    }
    var value = this.VectorMagnitude(scores);
    this.DrawMetricBar(x + width * 0.34, width * 0.32, zero, top, bottom, value, settings.overall);
    mgraphics.set_source_rgba(settings.label.r, settings.label.g, settings.label.b, settings.label.a);
    mgraphics.set_font_size(7);
    mgraphics.move_to(x + (width - this.EstimateLabelWidth("TOTAL")) * 0.5, bottom + 10);
    mgraphics.show_text("TOTAL");
};

SpectrumViewController.prototype.DrawGlobalMetricTable = function(width, height, settings) {
    var metrics = spectrumState.analysis.metrics;
    if (!metrics.length) return;
    var top = height - 43;
    var columnWidth = width / metrics.length;
    mgraphics.set_font_size(7);
    mgraphics.select_font_face("Arial");
    for (var index = 0; index < metrics.length; index++) {
        var metric = metrics[index];
        var x = index * columnWidth + 4;
        var metricColor = settings.metricColors[index % settings.metricColors.length];
        mgraphics.set_source_rgba(metricColor.r, metricColor.g, metricColor.b, metricColor.a);
        mgraphics.rectangle(x, top - 2, Math.max(8, columnWidth - 8), 2);
        mgraphics.fill();
        mgraphics.set_source_rgba(settings.label.r, settings.label.g, settings.label.b, settings.label.a);
        mgraphics.move_to(x, top + 7);
        mgraphics.show_text(this.AnalysisMetricLabel(metric.id));
        mgraphics.set_source_rgba(settings.current.r, settings.current.g, settings.current.b, 1);
        mgraphics.move_to(x, top + 19);
        mgraphics.show_text("C " + this.FormatAnalysisMetric(metric.id, metric.currentMean));
        mgraphics.set_source_rgba(settings.reference.r, settings.reference.g, settings.reference.b, 1);
        mgraphics.move_to(x, top + 31);
        mgraphics.show_text("R " + this.FormatAnalysisMetric(metric.id, metric.referenceMean));
    }
};

SpectrumViewController.prototype.BuildMetricDifference = function(
    id, current, currentDeviation, reference, referenceDeviation, band
) {
    var difference = Number(reference) - Number(current);
    var currentSpread = Math.max(0, Number(currentDeviation) || 0);
    var referenceSpread = Math.max(0, Number(referenceDeviation) || 0);
    var pooledSpread = Math.sqrt(
        (currentSpread * currentSpread + referenceSpread * referenceSpread) * 0.5);
    var scale = Math.max(this.MetricNormalizationFloor(id, band), pooledSpread);
    var standardizedMagnitude = Math.abs(difference) / scale *
        spectrumState.visualSettings.metrics.normalizationSensitivity;
    return {
        score: 1 - Math.exp(-standardizedMagnitude),
        direction: difference === 0 ? 0 : (difference > 0 ? 1 : -1)
    };
};

SpectrumViewController.prototype.MetricNormalizationFloor = function(id, band) {
    if (id === "rms_db" || id === "peak_db" || id === "transient_db") return 0.75;
    if (id === "crest_db") return 0.5;
    if (id === "centroid_hz") return Math.max(15, (band.maximumHz - band.minimumHz) * 0.015);
    if (id === "spectral_similarity") return 0.08;
    return 0.015;
};

SpectrumViewController.prototype.VectorMagnitude = function(values) {
    if (!values.length) return 0;
    var squaredSum = 0;
    for (var index = 0; index < values.length; index++) {
        squaredSum += values[index] * values[index];
    }
    return Math.sqrt(squaredSum / values.length);
};

SpectrumViewController.prototype.AnalysisMetricLabel = function(id) {
    var labels = {
        rms_db: "RMS", peak_db: "PEAK", crest_db: "CREST", centroid_hz: "CENTER",
        flatness: "FLAT", flux: "FLUX", transient_db: "TRANS", spectral_similarity: "MATCH"
    };
    return labels[id] || String(id).toUpperCase();
};

SpectrumViewController.prototype.FormatAnalysisMetric = function(id, value) {
    if (!isFinite(value)) return "--";
    if (id === "centroid_hz") return value >= 1000 ? (value / 1000).toFixed(1) + "k" : Math.round(value);
    if (id === "flatness" || id === "flux" || id === "spectral_similarity") return value.toFixed(2);
    return value.toFixed(1);
};

SpectrumViewController.prototype.FormatBandLabel = function(minimumHz, maximumHz) {
    function Format(value) { return value >= 1000 ? String(value / 1000) + "k" : String(value); }
    return Format(minimumHz) + "-" + Format(maximumHz);
};
