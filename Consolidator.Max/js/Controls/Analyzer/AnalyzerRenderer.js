const { UiColors } = require("../../Theme/UiColors.js");

class AnalyzerRenderer
{
    paint(graphics, presentation, layout, state)
    {
        graphics.set_source_rgba.apply(graphics, UiColors.analyzer.background);
        graphics.rectangle(0, 0, graphics.size[0], graphics.size[1]);
        graphics.fill();
        if (!presentation || !presentation.enabled) return;
        const drawSpectrum = (spectrum, color) => {
            const values = spectrum && spectrum.values;
            if (!values || values.length === 0) return;
            graphics.set_source_rgba.apply(graphics, color);
            graphics.set_line_width(1);
            graphics.new_path();
            for (let index = 0; index < values.length; index += 1) {
                let x = layout.left + layout.width * index / (values.length - 1);
                let y = layout.top + layout.height * values[index];
                if (index === 0) graphics.move_to(x, y);
                else graphics.line_to(x, y);
            }
            graphics.stroke();
        };
        drawSpectrum(presentation.spectrum, UiColors.analyzer.spectrum);
        drawSpectrum(presentation.referenceSpectrum, UiColors.analyzer.referenceSpectrum);
        drawSpectrum(presentation.differenceSpectrum, UiColors.analyzer.differenceSpectrum);
        const drawCurve = (values, color) => {
            if (!values || values.length === 0) return;
            graphics.set_source_rgba.apply(graphics, color);
            graphics.set_line_width(1.2);
            graphics.new_path();
            for (let index = 0; index < values.length; index += 1) {
                let x = layout.left + layout.width * index / (values.length - 1);
                let y = layout.top + layout.height * values[index];
                if (index === 0) graphics.move_to(x, y);
                else graphics.line_to(x, y);
            }
            graphics.stroke();
        };
        let allBanks = presentation.allBanksCurve;
        drawCurve(allBanks && allBanks.values,
            allBanks && allBanks.active === false
                ? UiColors.analyzer.allBanksInactive : UiColors.analyzer.allBanks);
        (presentation.curves || []).forEach((curve) => {
            let filterColors = UiColors.analyzer.filterCurves;
            let color = filterColors[(Number(curve.id) - 1) % filterColors.length];
            drawCurve(curve.values, curve.active === false
                ? UiColors.analyzer.filterInactive : color);
        });
        let combined = presentation.combinedCurve;
        drawCurve(combined && combined.values,
            combined && combined.active === false
                ? UiColors.analyzer.combinedInactive : UiColors.analyzer.combined);
        (presentation.handles || []).forEach((handle) => {
            if (!handle.enabled) return;
            let preview = state && state.preview[handle.id];
            let frequency = preview ? preview.x : handle.frequency;
            let gain = preview ? preview.y : handle.gain;
            let x = layout.left + layout.width * frequency;
            let y = layout.top + layout.height * gain;
            graphics.set_source_rgba.apply(graphics,
                presentation.scopeColor
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

module.exports = {
    AnalyzerRenderer: AnalyzerRenderer
};
