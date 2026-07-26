SpectrumViewController.prototype.Paint = function() {
    var size = mgraphics.size;
    var w = size[0];
    var h = size[1];
    var plotWidth = this.GetSpectrumPlotWidth(w);
    var plotBottom = this.GetPlotBottom(h);

    this.DrawBackground(w, h);
    this.DrawFrequencyGrid(plotWidth, plotBottom);
    this.DrawZeroLine(plotWidth, plotBottom);

    for (var i = 0; i < 2; i++) {
        var s = spectrumState.styles[i];
        this.DrawCurve(spectrumState.curves[i], plotWidth, plotBottom, s);
    }

    this.DrawFitCurve(plotWidth, plotBottom);
    this.DrawTotalEqResponse(plotWidth, plotBottom);
    this.DrawIndividualFilterCurves(plotWidth, plotBottom);

    this.DrawHandles(plotWidth, plotBottom);
    this.DrawFrequencyLabels(plotWidth, h);
    this.DrawDbRangeLabel(plotWidth, h);
}

SpectrumViewController.prototype.DrawFitCurve = function(w, plotBottom) {
    var values = spectrumState.fitCurve;
    if (!values || values.length < 2) return;

    var color = spectrumState.visualSettings.fitCurve;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_line_width(spectrumState.visualSettings.totalEqLineWidth);
    mgraphics.set_line_cap("round");
    mgraphics.new_path();
    for (var index = 1; index < values.length; index++) {
        var x0 = this.BinToX(index - 1, values.length, w);
        var y0 = this.DbToY(values[index - 1], plotBottom);
        var x1 = this.BinToX(index, values.length, w);
        var y1 = this.DbToY(values[index], plotBottom);
        mgraphics.move_to(x0, y0);
        mgraphics.line_to(x1, y1);
    }
    mgraphics.stroke();
}

SpectrumViewController.prototype.DrawIndividualFilterCurves = function(w, plotBottom) {
    for (var key in spectrumState.filterCurves) {
        if (!spectrumState.filterCurves.hasOwnProperty(key)) {
            continue;
        }

        var item = spectrumState.filterCurves[key];
        this.DrawAdaptiveFilterCurve(item.curve, w, plotBottom, item.color);
    }
}

SpectrumViewController.prototype.DrawTotalEqResponse = function(w, plotBottom) {
    var total = spectrumState.curves[4];
    if (!total || total.length < 2) {
        return;
    }

    var color = spectrumState.visualSettings.totalEqLineColor;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_line_width(spectrumState.visualSettings.totalEqLineWidth);
    mgraphics.new_path();

    for (var segment = 1; segment < total.length; segment++) {
        var x0 = this.BinToX(segment - 1, total.length, w);
        var y0 = this.DbToY(total[segment - 1], plotBottom);
        var x1 = this.BinToX(segment, total.length, w);
        var y1 = this.DbToY(total[segment], plotBottom);

        mgraphics.move_to(x0, y0);
        mgraphics.line_to(x1, y1);
    }
    mgraphics.stroke();
}

SpectrumViewController.prototype.DrawAdaptiveFilterCurve = function(values, w, plotBottom, filterColor, lineWidth, lineAlpha) {
    if (!values || values.length < 2) {
        return;
    }

    var alpha = Math.min(filterColor.a, lineAlpha || spectrumState.visualSettings.filterLineAlpha);
    mgraphics.set_source_rgba(filterColor.r, filterColor.g, filterColor.b, alpha);
    mgraphics.set_line_width(lineWidth || spectrumState.visualSettings.filterLineWidth);
    mgraphics.new_path();
    for (var i = 1; i < values.length; i++) {
        var x0 = this.BinToX(i - 1, values.length, w);
        var y0 = this.DbToY(values[i - 1], plotBottom);
        var x1 = this.BinToX(i, values.length, w);
        var y1 = this.DbToY(values[i], plotBottom);
        mgraphics.move_to(x0, y0);
        mgraphics.line_to(x1, y1);
    }
    mgraphics.stroke();
}

SpectrumViewController.prototype.DrawHandles = function(w, plotBottom) {
    var topHandle = spectrumState.selectedHandleSlot !== null
        ? this.GetHandleBySlot(spectrumState.selectedHandleSlot)
        : spectrumState.draggedHandle;
    for (var i = 0; i < spectrumState.handles.length; i++) {
        var item = spectrumState.handles[i];
        if (!item.active) {
            continue;
        }

        if (this.SameHandle(item, topHandle)) {
            continue;
        }

        this.DrawHandle(item, w, plotBottom, false, false);
    }

    if (topHandle && topHandle.active) {
    this.DrawHandle(topHandle, w, plotBottom, true, this.SameHandle(topHandle, spectrumState.draggedHandle));
    }
}

SpectrumViewController.prototype.DrawHandle = function(item, w, plotBottom, isSelected, isActive) {
        if (!item.active) {
            return;
        }

        var x = this.FrequencyToX(item.frequency, w);
        var y = this.DbToY(item.gain, plotBottom);
        var curve = spectrumState.filterCurves[item.slot];
        var color = curve ? curve.color : spectrumState.visualSettings.handleFallbackColor;
        var radius = isActive
            ? spectrumState.visualSettings.handleActiveRadius
            : (isSelected ? spectrumState.visualSettings.handleSelectedRadius : spectrumState.visualSettings.handleRadius);
        mgraphics.set_source_rgba(color.r, color.g, color.b, 1.0);
        mgraphics.ellipse(x - radius, y - radius, radius * 2, radius * 2);
        mgraphics.fill();
        if (isSelected || isActive) {
            var ring = isActive
                ? spectrumState.visualSettings.handleActiveRing
                : spectrumState.visualSettings.handleSelectedRing;
            mgraphics.set_source_rgba(ring.r, ring.g, ring.b, ring.a);
            mgraphics.set_line_width(spectrumState.visualSettings.handleRingWidth);
            mgraphics.ellipse(x - radius, y - radius, radius * 2, radius * 2);
            mgraphics.stroke();
        }
        mgraphics.set_source_rgba(
            color.r * spectrumState.visualSettings.handleInnerShade,
            color.g * spectrumState.visualSettings.handleInnerShade,
            color.b * spectrumState.visualSettings.handleInnerShade,
            1.0
        );
        var innerRadius = spectrumState.visualSettings.handleInnerRadius;
        mgraphics.ellipse(x - innerRadius, y - innerRadius, innerRadius * 2, innerRadius * 2);
        mgraphics.fill();
}

SpectrumViewController.prototype.DrawBackground = function(w, h) {
    var color = spectrumState.visualSettings.background;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.rectangle(0, 0, w, h);
    mgraphics.fill();
}

SpectrumViewController.prototype.DrawFrequencyGrid = function(w, plotBottom) {
    var gridStepDb = this.GetDbGridStep();
    var maxGridDb = Math.floor(spectrumState.maxDb / gridStepDb) * gridStepDb;
    var minGridDb = -maxGridDb;

    for (var db = minGridDb; db <= maxGridDb; db += gridStepDb) {
        if (db === 0) {
            continue;
        }

        this.DrawHorizontalGridLine(w, plotBottom, db);
        this.DrawDbLabel(w, plotBottom, db);
    }

    this.DrawDbLabel(w, plotBottom, 0);

    var minor = spectrumState.visualSettings.minorGrid;
    mgraphics.set_source_rgba(minor.r, minor.g, minor.b, minor.a);
    for (var j = 0; j < spectrumState.minorFrequencies.length; j++) {
        var minorX = this.FrequencyToX(spectrumState.minorFrequencies[j], w);
        mgraphics.set_line_width(minor.width);
        mgraphics.move_to(minorX, 0);
        mgraphics.line_to(minorX, plotBottom);
        mgraphics.stroke();
    }

    var major = spectrumState.visualSettings.majorGrid;
    mgraphics.set_source_rgba(major.r, major.g, major.b, major.a);
    mgraphics.set_line_width(major.width);
    for (var k = 0; k < spectrumState.majorFrequencies.length; k++) {
        var boundaryX = this.FrequencyToX(spectrumState.majorFrequencies[k], w);
        mgraphics.move_to(boundaryX, 0);
        mgraphics.line_to(boundaryX, plotBottom);
        mgraphics.stroke();
    }
}

SpectrumViewController.prototype.DrawZeroLine = function(w, plotBottom) {
    var y = this.DbToY(0, plotBottom);

    var color = spectrumState.visualSettings.zeroLine;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_line_width(color.width);
    mgraphics.move_to(0, y);
    mgraphics.line_to(w, y);
    mgraphics.stroke();
}

SpectrumViewController.prototype.DrawCurve = function(values, w, plotBottom, style) {
    if (!values || values.length < 2) {
        return;
    }

    var controller = this;
    var points = values.map(function (v, i) {
        return {
            x: controller.BinToX(i, values.length, w),
            y: controller.DbToY(v, plotBottom)
        };
    });

    if (style && style.fill) {
        this.DrawFilledCurve(points, w, plotBottom, style.fill);
    }

    if (style && style.outline) {
        this.DrawCurveOutline(points, style.outline);
    }
}

SpectrumViewController.prototype.DrawFilledCurve = function(points, w, plotBottom, fillStyle) {
    var gradient = this.CreateVerticalGradient(0, 0, 0, plotBottom, fillStyle);

    mgraphics.new_path();
    mgraphics.move_to(0, plotBottom);
    mgraphics.line_to(points[0].x, points[0].y);

    for (var i = 1; i < points.length - 1; i++) {
        var midX = (points[i].x + points[i + 1].x) * 0.5;
        var midY = (points[i].y + points[i + 1].y) * 0.5;

        mgraphics.curve_to(
            points[i].x, points[i].y,
            points[i].x, points[i].y,
            midX, midY
        );
    }

    var last = points[points.length - 1];
    mgraphics.line_to(last.x, last.y);
    mgraphics.line_to(w, plotBottom);
    mgraphics.close_path();

    if (gradient && mgraphics.set_source) {
        mgraphics.set_source(gradient);
    } else {
        mgraphics.set_source_rgba(fillStyle.r, fillStyle.g, fillStyle.b, fillStyle.a);
    }

    mgraphics.fill();
}

SpectrumViewController.prototype.DrawCurveOutline = function(points, outlineStyle) {
    mgraphics.new_path();
    mgraphics.move_to(points[0].x, points[0].y);

    for (var i = 1; i < points.length - 1; i++) {
        var midX = (points[i].x + points[i + 1].x) * 0.5;
        var midY = (points[i].y + points[i + 1].y) * 0.5;

        mgraphics.curve_to(
            points[i].x, points[i].y,
            points[i].x, points[i].y,
            midX, midY
        );
    }

    var last = points[points.length - 1];
    mgraphics.line_to(last.x, last.y);

    mgraphics.set_source_rgba(outlineStyle.r, outlineStyle.g, outlineStyle.b, outlineStyle.a);
    mgraphics.set_line_width(outlineStyle.width);
    mgraphics.stroke();
}

SpectrumViewController.prototype.DrawFrequencyLabels = function(w, h) {
    var labelY = h - 4;

    var color = spectrumState.visualSettings.frequencyLabel;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_font_size(color.size);
    mgraphics.select_font_face("Arial");

    var previousRight = -9999;

    for (var i = 0; i < spectrumState.frequencyLabels.length; i++) {
        var freq = spectrumState.frequencyLabels[i];
        if (!this.IsMajorFrequency(freq)) {
            continue;
        }

        var x = this.FrequencyToX(freq, w);
        var label = this.FormatFrequencyLabel(freq);
        var labelX = x - this.EstimateLabelWidth(label) * 0.5;
        var labelRight = labelX + this.EstimateLabelWidth(label);

        if (labelX < 0) {
            labelX = 0;
            labelRight = this.EstimateLabelWidth(label);
        }

        if (labelRight > w) {
            labelX = w - this.EstimateLabelWidth(label);
            labelRight = w;
        }

        if (labelX < previousRight + 6) {
            continue;
        }

        mgraphics.move_to(labelX, labelY);
        mgraphics.show_text(label);
        previousRight = labelRight;
    }
}

SpectrumViewController.prototype.DrawDbRangeLabel = function(w, h) {
    var label = spectrumState.dbRangeIndex >= 0 ? spectrumState.dbRangePresets[spectrumState.dbRangeIndex].label : (Math.abs(spectrumState.maxDb) + " dB");
    var text = "-" + label + " / +" + label;

    var color = spectrumState.visualSettings.rangeLabel;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_font_size(color.size);
    mgraphics.select_font_face("Arial");
    mgraphics.move_to(w - this.EstimateLabelWidth(text) - 6, 12);
    mgraphics.show_text(text);
}

SpectrumViewController.prototype.CreateVerticalGradient = function(x0, y0, x1, y1, fillStyle) {
    if (!mgraphics.pattern_create_linear) {
        return null;
    }

    var pattern = mgraphics.pattern_create_linear(x0, y0, x1, y1);
    if (!pattern || !pattern.add_color_stop_rgba) {
        return null;
    }

    pattern.add_color_stop_rgba(0, fillStyle.top.r, fillStyle.top.g, fillStyle.top.b, fillStyle.top.a);
    pattern.add_color_stop_rgba(1, fillStyle.bottom.r, fillStyle.bottom.g, fillStyle.bottom.b, fillStyle.bottom.a);
    return pattern;
}

SpectrumViewController.prototype.DrawHorizontalGridLine = function(w, plotBottom, db) {
    var y = this.DbToY(db, plotBottom);
    var color = spectrumState.visualSettings.horizontalGrid;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_line_width(color.width);
    mgraphics.move_to(0, y);
    mgraphics.line_to(w, y);
    mgraphics.stroke();
}

SpectrumViewController.prototype.DrawDbLabel = function(w, plotBottom, db) {
    var text = this.FormatDbLabel(db);
    var y = this.DbToY(db, plotBottom) + 3;

    var color = spectrumState.visualSettings.label;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_font_size(color.size);
    mgraphics.select_font_face("Arial");
    mgraphics.move_to(4, y);
    mgraphics.show_text(text);
}
// Curve aggregation and all drawing primitives.
