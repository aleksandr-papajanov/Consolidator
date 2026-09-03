const { UiColors } = require("../../Theme/UiColors.js");
const { DialControlOptions } = require("./DialControlOptions.js");

function clamp(value, minimum, maximum)
{
    return Math.max(minimum, Math.min(maximum, value));
}

class DialRenderer
{
    color(value, fallback)
    {
        return value && value.length >= 4 ? value : fallback;
    }

    displayValue(ring, normalizedValue)
    {
        let display = ring.display || {};
        let minimum = Number(display.minimum);
        let maximum = Number(display.maximum);
        let value = Number(normalizedValue);
        if (!isFinite(minimum) || !isFinite(maximum) || !isFinite(value)) {
            return display.value || "";
        }
        value = clamp(value, 0, 1);
        let physical = display.logarithmic && minimum > 0 && maximum > 0
            ? minimum * Math.pow(maximum / minimum, value)
            : minimum + (maximum - minimum) * value;
        let scale = isFinite(Number(display.scale)) ? Number(display.scale) : 1;
        let decimals = isFinite(Number(display.decimals))
            ? Number(display.decimals) : 2;
        return (physical * scale).toFixed(Math.max(0, Math.floor(decimals))) +
            String(display.suffix || "");
    }

    arc(centerX, centerY, radius, value, color, width)
    {
        this.arcRange(centerX, centerY, radius, 0, value, color, width);
    }

    arcRange(centerX, centerY, radius, startValue, endValue, color, width)
    {
        let options = DialControlOptions;
        let start = options.startAngle;
        let range = options.endAngle - start;
        let begin = start + range * clamp(startValue, 0, 1);
        let end = start + range * clamp(endValue, 0, 1);
        mgraphics.set_source_rgba.apply(
            mgraphics, this.color(color, options.ring));
        mgraphics.set_line_width(width);
        mgraphics.new_path();
        mgraphics.arc(centerX, centerY, radius, begin, end);
        mgraphics.stroke();
    }

    paintRing(presentation, ring, value, centerX, centerY, radius)
    {
        let options = DialControlOptions;
        let color = this.color(ring.color,
            presentation.enabled && presentation.active
                ? options.active : options.inactive);
        if (presentation.scopeColor && presentation.scopeColor.length >= 4) {
            color = presentation.scopeColor;
        }
        if (!presentation.groupScope) {
            this.arc(centerX, centerY, radius, 1, options.ring,
                options.lineWidth);
            this.arc(centerX, centerY, radius, value, color, options.lineWidth);
            this.paintVisualization(ring.visualization, centerX, centerY, radius);
            return;
        }

        let minimum = clamp(Number(ring.minimum), 0, 1);
        let maximum = clamp(Number(ring.maximum), 0, 1);
        let nextValue = clamp(Number(value), minimum, maximum);
        if (maximum > minimum) {
            this.arcRange(centerX, centerY, radius, minimum, maximum,
                options.ring, options.lineWidth);
            this.arcRange(centerX, centerY, radius, minimum, nextValue,
                color, options.lineWidth);
        }
        this.paintVisualization(ring.visualization, centerX, centerY, radius);
    }

    paintVisualization(visualization, centerX, centerY, radius)
    {
        if (!visualization) return;
        let value;
        if (visualization.type === "level") {
            this.arc(centerX, centerY,
                radius + DialControlOptions.indicatorWidth * 2,
                clamp(Number(visualization.peak), 0, 1),
                DialControlOptions.visualization,
                DialControlOptions.indicatorWidth);
            value = visualization.smoothed;
        }
        else if (visualization.type === "relative") {
            value = Math.abs(visualization.value);
        }
        else if (visualization.type === "reduction" ||
                visualization.type === "saturation") {
            value = visualization.value;
        }
        else return;

        this.arc(centerX, centerY,
            radius + DialControlOptions.indicatorWidth,
            clamp(Number(value), 0, 1),
            DialControlOptions.visualization,
            DialControlOptions.indicatorWidth);
    }

    paint(presentation, previewValues, showValueLabel)
    {
        let width = mgraphics.size[0];
        let height = mgraphics.size[1];
        let centerX = width * 0.5;
        let centerY = height * 0.55;
        let radius = Math.max(1, Math.min(width, height) * 0.38);
        mgraphics.set_source_rgba.apply(
            mgraphics, DialControlOptions.background);
        mgraphics.rectangle(0, 0, width, height);
        mgraphics.fill();

        let ring = (presentation.rings || [])[0];
        if (!ring) return;
        let value = previewValues[0] === undefined
            ? ring.value : previewValues[0];
        this.paintRing(presentation, ring, value, centerX, centerY, radius);
        if (!presentation.label) return;
        mgraphics.select_font_face(UiColors.typography.controlLabelFontFamily);
        mgraphics.set_font_size(UiColors.typography.controlLabelFontSize);
        mgraphics.set_source_rgba.apply(mgraphics, UiColors.base.text);
        let label = showValueLabel && ring.display
            ? this.displayValue(ring, value) : String(presentation.label);
        let textSize = mgraphics.text_measure(label);
        mgraphics.move_to((width - textSize[0]) * 0.5, height - 7);
        mgraphics.show_text(label);
    }
}

module.exports = {
    DialRenderer: DialRenderer,
    clampDialValue: clamp
};
