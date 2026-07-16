function paint() {
    var size = mgraphics.size;
    var w = size[0];
    var h = size[1];
    var plotBottom = getPlotBottom(h);

    drawBackground(w, h);
    drawFrequencyGrid(w, plotBottom);
    drawZeroLine(w, plotBottom);

    for (var i = 0; i < 3; i++) {
        var s = styles[i];
        drawCurve(curves[i], w, plotBottom, s);
    }

    drawIndividualFilterCurves(w, plotBottom);
    drawTotalFilterCurve(w, plotBottom);
    drawSelectedFilterCurve(w, plotBottom);

    drawHandles(w, plotBottom);
    drawFrequencyLabels(w, h);
    drawDbRangeLabel(w, h);
}

function drawIndividualFilterCurves(w, plotBottom) {
    for (var key in filterCurves) {
        if (!filterCurves.hasOwnProperty(key)) {
            continue;
        }

        var item = filterCurves[key];
        drawAdaptiveFilterCurve(item.curve, w, plotBottom, item.color);
    }
}

function drawSelectedFilterCurve(w, plotBottom) {
    var selected = selectedHandleSlot !== null
        ? getHandleBySlot(selectedHandleSlot)
        : draggedHandle;
    if (!selected || !filterCurves[selected.slot]) {
        return;
    }

    drawAdaptiveFilterCurve(
        filterCurves[selected.slot].curve,
        w,
        plotBottom,
        filterCurves[selected.slot].color,
        visualSettings.selectedFilterLineWidth,
        visualSettings.selectedFilterLineAlpha
    );
}

function drawTotalFilterCurve(w, plotBottom) {
    var items = [];
    for (var key in filterCurves) {
        if (filterCurves.hasOwnProperty(key)) {
            items.push(filterCurves[key]);
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
        var x0 = binToX(segment - 1, total.length, w);
        var y0 = dbToY(total[segment - 1], plotBottom);
        var x1 = binToX(segment, total.length, w);
        var y1 = dbToY(total[segment], plotBottom);
        var color = totalColor(items, segment - 1, total[segment - 1]);

        mgraphics.set_source_rgba(color.r, color.g, color.b, 1.0);
        mgraphics.set_line_width(visualSettings.totalLineWidth);
        mgraphics.move_to(x0, y0);
        mgraphics.line_to(x1, y1);
        mgraphics.stroke();
    }
}

function drawAdaptiveFilterCurve(values, w, plotBottom, filterColor, lineWidth, lineAlpha) {
    if (!values || values.length < 2) {
        return;
    }

    for (var i = 1; i < values.length; i++) {
        var x0 = binToX(i - 1, values.length, w);
        var y0 = dbToY(values[i - 1], plotBottom);
        var x1 = binToX(i, values.length, w);
        var y1 = dbToY(values[i], plotBottom);
        var magnitude = (Math.abs(values[i - 1]) + Math.abs(values[i])) * 0.5;
        var rawStrength = clamp(
            magnitude / visualSettings.filterColorTransitionDb,
            0,
            1
        );
        var strength = 1 - Math.pow(
            1 - rawStrength,
            visualSettings.filterColorSensitivity
        );
        var color = {
            r: visualSettings.totalBaseColor.r + strength * (filterColor.r - visualSettings.totalBaseColor.r),
            g: visualSettings.totalBaseColor.g + strength * (filterColor.g - visualSettings.totalBaseColor.g),
            b: visualSettings.totalBaseColor.b + strength * (filterColor.b - visualSettings.totalBaseColor.b),
            a: Math.min(filterColor.a, lineAlpha || visualSettings.filterLineAlpha)
        };

        mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
        mgraphics.set_line_width(lineWidth || visualSettings.filterLineWidth);
        mgraphics.move_to(x0, y0);
        mgraphics.line_to(x1, y1);
        mgraphics.stroke();
    }
}

function totalColor(items, index, net) {
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
        return visualSettings.totalBaseColor;
    }

    var rawNetStrength = clamp(
        Math.abs(net) / visualSettings.totalColorTransitionDb,
        0,
        1
    );
    var netStrength = 1 - Math.pow(
        1 - rawNetStrength,
        visualSettings.totalColorNetSensitivity
    );
    var strength = netStrength;
    var base = {
        r: weighted.r / absolute,
        g: weighted.g / absolute,
        b: weighted.b / absolute
    };
    return {
        r: visualSettings.totalBaseColor.r + strength * (base.r - visualSettings.totalBaseColor.r),
        g: visualSettings.totalBaseColor.g + strength * (base.g - visualSettings.totalBaseColor.g),
        b: visualSettings.totalBaseColor.b + strength * (base.b - visualSettings.totalBaseColor.b)
    };
}

function drawHandles(w, plotBottom) {
    var topHandle = selectedHandleSlot !== null
        ? getHandleBySlot(selectedHandleSlot)
        : draggedHandle;
    for (var i = 0; i < handles.length; i++) {
        var item = handles[i];
        if (!item.active) {
            continue;
        }

        if (sameHandle(item, topHandle)) {
            continue;
        }

        drawHandle(item, w, plotBottom, false, false);
    }

    if (topHandle && topHandle.active) {
    drawHandle(topHandle, w, plotBottom, true, sameHandle(topHandle, draggedHandle));
    }
}

function drawHandle(item, w, plotBottom, isSelected, isActive) {
        if (!item.active) {
            return;
        }

        var x = frequencyToX(item.frequency, w);
        var y = dbToY(item.gain, plotBottom);
        var curve = filterCurves[item.slot];
        var color = curve ? curve.color : visualSettings.handleFallbackColor;
        var radius = isActive
            ? visualSettings.handleActiveRadius
            : (isSelected ? visualSettings.handleSelectedRadius : visualSettings.handleRadius);
        mgraphics.set_source_rgba(color.r, color.g, color.b, 1.0);
        mgraphics.ellipse(x - radius, y - radius, radius * 2, radius * 2);
        mgraphics.fill();
        if (isSelected || isActive) {
            var ring = isActive
                ? visualSettings.handleActiveRing
                : visualSettings.handleSelectedRing;
            mgraphics.set_source_rgba(ring.r, ring.g, ring.b, ring.a);
            mgraphics.set_line_width(visualSettings.handleRingWidth);
            mgraphics.ellipse(x - radius, y - radius, radius * 2, radius * 2);
            mgraphics.stroke();
        }
        mgraphics.set_source_rgba(
            color.r * visualSettings.handleInnerShade,
            color.g * visualSettings.handleInnerShade,
            color.b * visualSettings.handleInnerShade,
            1.0
        );
        var innerRadius = visualSettings.handleInnerRadius;
        mgraphics.ellipse(x - innerRadius, y - innerRadius, innerRadius * 2, innerRadius * 2);
        mgraphics.fill();
}

function drawBackground(w, h) {
    var color = visualSettings.background;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.rectangle(0, 0, w, h);
    mgraphics.fill();
}

function drawFrequencyGrid(w, plotBottom) {
    var gridStepDb = getDbGridStep();
    var maxGridDb = Math.floor(maxDb / gridStepDb) * gridStepDb;
    var minGridDb = -maxGridDb;

    for (var db = minGridDb; db <= maxGridDb; db += gridStepDb) {
        if (db === 0) {
            continue;
        }

        drawHorizontalGridLine(w, plotBottom, db);
        drawDbLabel(w, plotBottom, db);
    }

    drawDbLabel(w, plotBottom, 0);

    var minor = visualSettings.minorGrid;
    mgraphics.set_source_rgba(minor.r, minor.g, minor.b, minor.a);
    for (var j = 0; j < minorFrequencies.length; j++) {
        var minorX = frequencyToX(minorFrequencies[j], w);
        mgraphics.set_line_width(minor.width);
        mgraphics.move_to(minorX, 0);
        mgraphics.line_to(minorX, plotBottom);
        mgraphics.stroke();
    }

    var major = visualSettings.majorGrid;
    mgraphics.set_source_rgba(major.r, major.g, major.b, major.a);
    mgraphics.set_line_width(major.width);
    for (var k = 0; k < majorFrequencies.length; k++) {
        var boundaryX = frequencyToX(majorFrequencies[k], w);
        mgraphics.move_to(boundaryX, 0);
        mgraphics.line_to(boundaryX, plotBottom);
        mgraphics.stroke();
    }
}

function drawZeroLine(w, plotBottom) {
    var y = dbToY(0, plotBottom);

    var color = visualSettings.zeroLine;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_line_width(color.width);
    mgraphics.move_to(0, y);
    mgraphics.line_to(w, y);
    mgraphics.stroke();
}

function drawCurve(values, w, plotBottom, style) {
    if (!values || values.length < 2) {
        return;
    }

    var points = values.map(function (v, i) {
        return {
            x: binToX(i, values.length, w),
            y: dbToY(v, plotBottom)
        };
    });

    if (style && style.fill) {
        drawFilledCurve(points, w, plotBottom, style.fill);
    }

    if (style && style.outline) {
        drawCurveOutline(points, style.outline);
    }
}

function drawFilledCurve(points, w, plotBottom, fillStyle) {
    var gradient = createVerticalGradient(0, 0, 0, plotBottom, fillStyle);

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

function drawCurveOutline(points, outlineStyle) {
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

function drawFrequencyLabels(w, h) {
    var labelY = h - 4;

    var color = visualSettings.frequencyLabel;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_font_size(color.size);
    mgraphics.select_font_face("Arial");

    var previousRight = -9999;

    for (var i = 0; i < frequencyLabels.length; i++) {
        var freq = frequencyLabels[i];
        if (!isMajorFrequency(freq)) {
            continue;
        }

        var x = frequencyToX(freq, w);
        var label = formatFrequencyLabel(freq);
        var labelX = x - estimateLabelWidth(label) * 0.5;
        var labelRight = labelX + estimateLabelWidth(label);

        if (labelX < 0) {
            labelX = 0;
            labelRight = estimateLabelWidth(label);
        }

        if (labelRight > w) {
            labelX = w - estimateLabelWidth(label);
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

function drawDbRangeLabel(w, h) {
    var label = dbRangeIndex >= 0 ? dbRangePresets[dbRangeIndex].label : (Math.abs(maxDb) + " dB");
    var text = "-" + label + " / +" + label;

    var color = visualSettings.rangeLabel;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_font_size(color.size);
    mgraphics.select_font_face("Arial");
    mgraphics.move_to(w - estimateLabelWidth(text) - 6, 12);
    mgraphics.show_text(text);
}

function createVerticalGradient(x0, y0, x1, y1, fillStyle) {
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

function drawHorizontalGridLine(w, plotBottom, db) {
    var y = dbToY(db, plotBottom);
    var color = visualSettings.horizontalGrid;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_line_width(color.width);
    mgraphics.move_to(0, y);
    mgraphics.line_to(w, y);
    mgraphics.stroke();
}

function drawDbLabel(w, plotBottom, db) {
    var text = formatDbLabel(db);
    var y = dbToY(db, plotBottom) + 3;

    var color = visualSettings.label;
    mgraphics.set_source_rgba(color.r, color.g, color.b, color.a);
    mgraphics.set_font_size(color.size);
    mgraphics.select_font_face("Arial");
    mgraphics.move_to(4, y);
    mgraphics.show_text(text);
}
// Curve aggregation and all drawing primitives.
