const { UiColors } = require("../../../Shared/Theme/UiColors.js");

class AnalyzerRenderer
{
    paint(graphics, presentation, layout)
    {
        graphics.set_source_rgba.apply(graphics, UiColors.analyzer.background);
        graphics.rectangle(0, 0, graphics.size[0], graphics.size[1]);
        graphics.fill();
        if (!presentation || !presentation.enabled) return;
        drawFrequencyGrid(graphics, layout, presentation);
        drawRoughGrid(graphics, layout, presentation);
        const drawSpectrum = (spectrum, color) => {
            const values = spectrum && spectrum.values;
            if (!values || values.length === 0) return;
            let fftSize = Number(spectrum.fftSize);
            let sampleRate = Number(spectrum.sampleRate);
            if (!isFinite(fftSize) || fftSize <= 0 ||
                    !isFinite(sampleRate) || sampleRate <= 0) return;
            let minimumFrequency = Number(presentation.frequencyMinimum);
            let maximumFrequency = Number(presentation.frequencyMaximum);
            if (!isFinite(minimumFrequency) || minimumFrequency <= 0 ||
                    !isFinite(maximumFrequency) || maximumFrequency <= minimumFrequency) {
                return;
            }
            let renderedValues = resampleSpectrum(
                values,
                fftSize,
                sampleRate,
                minimumFrequency,
                maximumFrequency,
                256
            );
            graphics.set_source_rgba.apply(graphics, color);
            graphics.set_line_width(1);
            graphics.new_path();
            let drawing = false;
            for (let index = 0; index < renderedValues.length; index += 1) {
                let x = layout.left + layout.width * index /
                    (renderedValues.length - 1);
                let value = Number(renderedValues[index]);
                if (!isFinite(value) || value < 0 || value > 1) {
                    drawing = false;
                    continue;
                }
                let y = layout.top + layout.height * value;
                if (!drawing) {
                    graphics.move_to(x, y);
                    drawing = true;
                }
                else {
                    graphics.line_to(x, y);
                }
            }
            graphics.stroke();
        };
        drawSpectrum(presentation.spectrum, UiColors.analyzer.spectrum);
        drawSpectrum(
            presentation.groupSpectrum,
            presentation.scopeColor || UiColors.analyzer.groupSpectrum
        );
        const drawCurve = (values, color) => {
            if (!values || values.length === 0) return;
            graphics.set_source_rgba.apply(graphics, color);
            graphics.set_line_width(1.2);
            graphics.new_path();
            let drawing = false;
            for (let index = 0; index < values.length; index += 1) {
                let x = layout.left + layout.width * index / (values.length - 1);
                let value = Number(values[index]);
                if (!isFinite(value) || value < 0 || value > 1) {
                    drawing = false;
                    continue;
                }
                let y = layout.top + layout.height * value;
                if (!drawing) {
                    graphics.move_to(x, y);
                    drawing = true;
                }
                else {
                    graphics.line_to(x, y);
                }
            }
            graphics.stroke();
        };
        (presentation.curves || []).forEach((curve) => {
            let filterColors = UiColors.analyzer.filterCurves;
            let color = filterColors[(Number(curve.id) - 1) % filterColors.length];
            drawCurve(curve.values, curve.active === false
                ? UiColors.analyzer.filterInactive : color);
        });
        drawCurve(
            presentation.allBanksCurve && presentation.allBanksCurve.values,
            UiColors.analyzer.allBanksCurve
        );
        let combined = presentation.combinedCurve;
        drawCurve(combined && combined.values,
            combined && combined.active === false
                ? UiColors.analyzer.combinedInactive : UiColors.analyzer.combined);
        (presentation.handles || []).forEach((handle) => {
            if (!handle.enabled) return;
            let frequency = handle.frequency;
            let gain = handle.gain;
            let x = layout.left + layout.width * frequency;
            let y = layout.top + layout.height * gain;
            graphics.set_source_rgba.apply(graphics,
                presentation.scopeActive && presentation.scopeColor
                    ? presentation.scopeColor
                    : handle.selected ? UiColors.analyzer.selectedHandle : UiColors.analyzer.handle);
            if (handle.capabilities && !handle.capabilities.frequency) {
                graphics.new_path();
                graphics.move_to(x - 6, y);
                graphics.line_to(x + 3, y - 5);
                graphics.line_to(x + 3, y + 5);
                graphics.close_path();
                graphics.fill();
            }
            else {
                graphics.ellipse(x - 4, y - 4, 8, 8);
                graphics.fill();
            }
        });
    }
}

function drawRoughGrid(graphics, layout, presentation)
{
    if (!presentation.precisionRough || !presentation.roughGrid) return;
    const frequencies = presentation.roughGrid.frequencies || [];
    const gains = presentation.roughGrid.gains || [];
    graphics.set_source_rgba.apply(graphics, UiColors.analyzer.grid);
    for (let frequencyIndex = 0;
        frequencyIndex < frequencies.length; frequencyIndex += 1) {
        for (let gainIndex = 0; gainIndex < gains.length; gainIndex += 1) {
            let x = layout.left + layout.width * frequencies[frequencyIndex];
            let y = layout.top + layout.height * gains[gainIndex];
            graphics.ellipse(x - 1, y - 1, 2, 2);
            graphics.fill();
        }
    }
}

function drawFrequencyGrid(graphics, layout, presentation)
{
    const frequencies = [100, 200, 500, 1000, 2000, 5000, 10000];
    let minimumFrequency = Math.max(1, Number(presentation.frequencyMinimum));
    let maximumFrequency = Math.max(minimumFrequency, Number(presentation.frequencyMaximum));
    let logMinimum = Math.log(minimumFrequency);
    let logRange = Math.log(maximumFrequency) - logMinimum;
    if (logRange <= 0) return;

    graphics.set_source_rgba.apply(graphics, UiColors.analyzer.grid);
    graphics.set_line_width(1);
    graphics.new_path();
    frequencies.forEach((frequency) => {
        if (frequency < minimumFrequency || frequency > maximumFrequency) return;
        let normalized = (Math.log(frequency) - logMinimum) / logRange;
        let x = Math.round(layout.left + layout.width * normalized) + 0.5;
        graphics.move_to(x, layout.top);
        graphics.line_to(x, layout.top + layout.height);
    });
    graphics.stroke();
}

function resampleSpectrum(
    values,
    fftSize,
    sampleRate,
    minimumFrequency,
    maximumFrequency,
    pointCount)
{
    let result = new Array(pointCount);
    let logMinimum = Math.log(minimumFrequency);
    let logRange = Math.log(maximumFrequency) - logMinimum;
    let maximumBin = values.length - 1;
    for (let point = 0; point < pointCount; point += 1) {
        let normalized = pointCount > 1
            ? point / (pointCount - 1) : 0;
        let frequency = Math.exp(logMinimum + normalized * logRange);
        let bin = frequency * fftSize / sampleRate;
        let lowerBin = Math.max(0, Math.min(maximumBin, Math.floor(bin)));
        let upperBin = Math.min(maximumBin, lowerBin + 1);
        let fraction = bin - lowerBin;
        let lowerValue = Number(values[lowerBin]);
        let upperValue = Number(values[upperBin]);
        if (!isFinite(lowerValue)) lowerValue = 1;
        if (!isFinite(upperValue)) upperValue = lowerValue;
        result[point] = lowerValue + (upperValue - lowerValue) * fraction;
    }
    return result;
}

module.exports = {
    AnalyzerRenderer: AnalyzerRenderer
};
