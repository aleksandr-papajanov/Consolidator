SpectrumViewController.prototype.paint = function() {
    var size = mgraphics.size;
    var w = size[0];
    var h = size[1];
    var plotBottom = this.getPlotBottom(h);

    this.drawBackground(w, h);
    this.drawFrequencyGrid(w, plotBottom);
    this.drawZeroLine(w, plotBottom);

    for (var i = 0; i < 3; i++) {
        var s = spectrumState.styles[i];
        this.drawCurve(spectrumState.curves[i], w, plotBottom, s);
    }

    this.drawIndividualFilterCurves(w, plotBottom);
    this.drawTotalFilterCurve(w, plotBottom);
    this.drawSelectedFilterCurve(w, plotBottom);

    this.drawHandles(w, plotBottom);
    this.drawFrequencyLabels(w, h);
    this.drawDbRangeLabel(w, h);
}

SpectrumViewController.prototype.drawIndividualFilterCurves = function(w, plotBottom) {
    for (var key in spectrumState.filterCurves) {
        if (!spectrumState.filterCurves.hasOwnProperty(key)) {
            continue;
        }

        var item = spectrumState.filterCurves[key];
        this.drawAdaptiveFilterCurve(item.curve, w, plotBottom, item.color);
    }
}

SpectrumViewController.prototype.drawSelectedFilterCurve = function(w, plotBottom) {
    var selected = spectrumState.selectedHandleSlot !== null
        ? this.getHandleBySlot(spectrumState.selectedHandleSlot)
        : spectrumState.draggedHandle;
    if (!selected || !spectrumState.filterCurves[selected.slot]) {
        return;
    }

    this.drawAdaptiveFilterCurve(
        spectrumState.filterCurves[selected.slot].curve,
        w,
        plotBottom,
        spectrumState.filterCurves[selected.slot].color,
        spectrumState.visualSettings.selectedFilterLineWidth,
        spectrumState.visualSettings.selectedFilterLineAlpha
    );
}

SpectrumViewController.prototype.drawTotalFilterCurve = function(w, plotBottom) {
    var items = [];
    for (var key in spectrumState.filterCurves) {
        if (spectrumState.filterCurves.hasOwnProperty(key)) {
            items.push(spectrumState.filterCurves[key]);
        }
    }
    if (items.length === 0) {
        return;
    }

    var count = 0;
    for (var i = 0; i < items.length; i++) {
        count = Math.max(count, items[i].curve.length);
    }

    var total = [];
    for (var index = 0; index < count; index++) {
        var sum = 0;
        for (var itemIndex = 0; itemIndex < items.length; itemIndex++) {
            sum += Number(items[itemIndex].curve[index] || 0);
        }
        total.push(sum);
    }

    for (var segment = 1; segment < total.length; segment++) {
        var x0 = this.binToX(segment - 1, total.length, w);
        var y0 = this.dbToY(total[segment - 1], plotBottom);
        var x1 = this.binToX(segment, total.length, w);
        var y1 = this.dbToY(total[segment], plotBottom);
        var color = this.totalColor(items, segment - 1, total[segment - 1]);

        mgraphics.set_source_rgba(color.r, color.g, color.b, 1.0);
        mgraphics.set_line_width(spectrumState.visualSettings.totalLineWidth);
        mgraphics.move_to(x0, y0);
        mgraphics.line_to(x1, y1);
        mgraphics.stroke();
    }
}

SpectrumViewController.prototype.drawAdaptiveFilterCurve = function(values, w, plotBottom, filterColor, lineWidth, lineAlpha) {
    if (!values || values.length < 2) {
        return;
    }

    for (var i = 1; i < values.length; i++) {
        var x0 = this.binToX(i - 1, values.length, w);
        var y0 = this.dbToY(values[i - 1], plotBottom);
        var x1 = this.binToX(i, values.length, w);
        var y1 = this.dbToY(values[i], plotBottom);
        var magnitude = (Math.abs(values[i - 1]) + Math.abs(values[i])) * 0.5;
        var rawStrength = this.clamp(
            magnitude / spectrumState.visualSettings.filterColorTransitionDb,
            0,
            1
        );
        var strength = 1 - Math.pow(
            1 - rawStrength,
            spectrumState.visualSettings.filterColorSensitivity
        );
        var color = {
            r: spectrumState.visualSettings.totalBaseColor.r + strength * (filterColor.r - spectrumState.visualSettings.totalBaseColor.r),
            g: spectrumState.visualSettings.totalBaseColor.g + strength * (filterColor.g - spectrumState.visualSettings.totalBaseColor.g),
            b: spectrumState.visualSettings.totalBaseColor.b + strength * (filterColor.b - spectrumState.visualSettings.totalBaseColor.b),
            a: Math.min(filterColor.a, lineAlpha || spectrumState.visualSettings.filterLineAlpha)
        };

        mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
        mgraphics.set_line_width(lineWidth || spectrumState.visualSettings.filterLineWidth);
        mgraphics.move_to(x0, y0);
        mgraphics.line_to(x1, y1);
        mgraphics.stroke();
    }
}

SpectrumViewController.prototype.totalColor = function(items, index, net) {
    var weighted = { r: 0, g: 0, b: 0 };
    var absolute = 0;
    for (var i = 0; i < items.length; i++) {
        var contribution = Math.abs(Number(items[i].curve[index] || 0));
        weighted.r += items[i].color.r * contribution;
        weighted.g += items[i].color.g * contribution;
        weighted.b += items[i].color.b * contribution;
        absolute += contribution;
    }

    if (absolute < 0.001) {
        return spectrumState.visualSettings.totalBaseColor;
    }

    var rawNetStrength = this.clamp(
        Math.abs(net) / spectrumState.visualSettings.totalColorTransitionDb,
        0,
        1
    );
    var netStrength = 1 - Math.pow(
        1 - rawNetStrength,
        spectrumState.visualSettings.totalColorNetSensitivity
    );
    var strength = netStrength;
    var base = {
        r: weighted.r / absolute,
        g: weighted.g / absolute,
        b: weighted.b / absolute
    };
    return {
        r: spectrumState.visualSettings.totalBaseColor.r + strength * (base.r - spectrumState.visualSettings.totalBaseColor.r),
        g: spectrumState.visualSettings.totalBaseColor.g + strength * (base.g - spectrumState.visualSettings.totalBaseColor.g),
        b: spectrumState.visualSettings.totalBaseColor.b + strength * (base.b - spectrumState.visualSettings.totalBaseColor.b)
    };
}

SpectrumViewController.prototype.drawHandles = function(w, plotBottom) {
    var topHandle = spectrumState.selectedHandleSlot !== null
        ? this.getHandleBySlot(spectrumState.selectedHandleSlot)
        : spectrumState.draggedHandle;
    for (var i = 0; i < spectrumState.handles.length; i++) {
        var item = spectrumState.handles[i];
        if (!item.active) {
            continue;
        }

        if (this.sameHandle(item, topHandle)) {
            continue;
        }

        this.drawHandle(item, w, plotBottom, false, false);
    }

    if (topHandle && topHandle.active) {
    this.drawHandle(topHandle, w, plotBottom, true, this.sameHandle(topHandle, spectrumState.draggedHandle));
    }
}

SpectrumViewController.prototype.drawHandle = function(item, w, plotBottom, isSelected, isActive) {
        if (!item.active) {
            return;
        }

        var x = this.frequencyToX(item.frequency, w);
        var y = this.dbToY(item.gain, plotBottom);
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

SpectrumViewController.prototype.drawBackground = function(w, h) {
    var color = spectrumState.visualSettings.background;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.rectangle(0, 0, w, h);
    mgraphics.fill();
}

SpectrumViewController.prototype.drawFrequencyGrid = function(w, plotBottom) {
    var gridStepDb = this.getDbGridStep();
    var maxGridDb = Math.floor(spectrumState.maxDb / gridStepDb) * gridStepDb;
    var minGridDb = -maxGridDb;

    for (var db = minGridDb; db <= maxGridDb; db += gridStepDb) {
        if (db === 0) {
            continue;
        }

        this.drawHorizontalGridLine(w, plotBottom, db);
        this.drawDbLabel(w, plotBottom, db);
    }

    this.drawDbLabel(w, plotBottom, 0);

    var minor = spectrumState.visualSettings.minorGrid;
    mgraphics.set_source_rgba(minor.r, minor.g, minor.b, minor.a);
    for (var j = 0; j < spectrumState.minorFrequencies.length; j++) {
        var minorX = this.frequencyToX(spectrumState.minorFrequencies[j], w);
        mgraphics.set_line_width(minor.width);
        mgraphics.move_to(minorX, 0);
        mgraphics.line_to(minorX, plotBottom);
        mgraphics.stroke();
    }

    var major = spectrumState.visualSettings.majorGrid;
    mgraphics.set_source_rgba(major.r, major.g, major.b, major.a);
    mgraphics.set_line_width(major.width);
    for (var k = 0; k < spectrumState.majorFrequencies.length; k++) {
        var boundaryX = this.frequencyToX(spectrumState.majorFrequencies[k], w);
        mgraphics.move_to(boundaryX, 0);
        mgraphics.line_to(boundaryX, plotBottom);
        mgraphics.stroke();
    }
}

SpectrumViewController.prototype.drawZeroLine = function(w, plotBottom) {
    var y = this.dbToY(0, plotBottom);

    var color = spectrumState.visualSettings.zeroLine;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_line_width(color.width);
    mgraphics.move_to(0, y);
    mgraphics.line_to(w, y);
    mgraphics.stroke();
}

SpectrumViewController.prototype.drawCurve = function(values, w, plotBottom, style) {
    if (!values || values.length < 2) {
        return;
    }

    var controller = this;
    var points = values.map(function (v, i) {
        return {
            x: controller.binToX(i, values.length, w),
            y: controller.dbToY(v, plotBottom)
        };
    });

    if (style && style.fill) {
        this.drawFilledCurve(points, w, plotBottom, style.fill);
    }

    if (style && style.outline) {
        this.drawCurveOutline(points, style.outline);
    }
}

SpectrumViewController.prototype.drawFilledCurve = function(points, w, plotBottom, fillStyle) {
    var gradient = this.createVerticalGradient(0, 0, 0, plotBottom, fillStyle);

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

SpectrumViewController.prototype.drawCurveOutline = function(points, outlineStyle) {
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

SpectrumViewController.prototype.drawFrequencyLabels = function(w, h) {
    var labelY = h - 4;

    var color = spectrumState.visualSettings.frequencyLabel;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_font_size(color.size);
    mgraphics.select_font_face("Arial");

    var previousRight = -9999;

    for (var i = 0; i < spectrumState.frequencyLabels.length; i++) {
        var freq = spectrumState.frequencyLabels[i];
        if (!this.isMajorFrequency(freq)) {
            continue;
        }

        var x = this.frequencyToX(freq, w);
        var label = this.formatFrequencyLabel(freq);
        var labelX = x - this.estimateLabelWidth(label) * 0.5;
        var labelRight = labelX + this.estimateLabelWidth(label);

        if (labelX < 0) {
            labelX = 0;
            labelRight = this.estimateLabelWidth(label);
        }

        if (labelRight > w) {
            labelX = w - this.estimateLabelWidth(label);
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

SpectrumViewController.prototype.drawDbRangeLabel = function(w, h) {
    var label = spectrumState.dbRangeIndex >= 0 ? spectrumState.dbRangePresets[spectrumState.dbRangeIndex].label : (Math.abs(spectrumState.maxDb) + " dB");
    var text = "-" + label + " / +" + label;

    var color = spectrumState.visualSettings.rangeLabel;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_font_size(color.size);
    mgraphics.select_font_face("Arial");
    mgraphics.move_to(w - this.estimateLabelWidth(text) - 6, 12);
    mgraphics.show_text(text);
}

SpectrumViewController.prototype.createVerticalGradient = function(x0, y0, x1, y1, fillStyle) {
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

SpectrumViewController.prototype.drawHorizontalGridLine = function(w, plotBottom, db) {
    var y = this.dbToY(db, plotBottom);
    var color = spectrumState.visualSettings.horizontalGrid;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_line_width(color.width);
    mgraphics.move_to(0, y);
    mgraphics.line_to(w, y);
    mgraphics.stroke();
}

SpectrumViewController.prototype.drawDbLabel = function(w, plotBottom, db) {
    var text = this.formatDbLabel(db);
    var y = this.dbToY(db, plotBottom) + 3;

    var color = spectrumState.visualSettings.label;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_font_size(color.size);
    mgraphics.select_font_face("Arial");
    mgraphics.move_to(4, y);
    mgraphics.show_text(text);
}
// Curve aggregation and all drawing primitives.
