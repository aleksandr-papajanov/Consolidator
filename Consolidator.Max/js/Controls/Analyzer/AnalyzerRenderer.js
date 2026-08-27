class AnalyzerRenderer
{
    paint(graphics, presentation, layout, state)
    {
        graphics.set_source_rgba(0.06, 0.06, 0.07, 1);
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
        drawSpectrum(presentation.spectrum, [0.85, 0.85, 0.9, 0.7]);
        drawSpectrum(presentation.referenceSpectrum, [0.55, 0.65, 0.8, 0.55]);
        drawSpectrum(presentation.differenceSpectrum, [0.9, 0.45, 0.35, 0.65]);
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
        (presentation.curves || []).forEach((curve) => {
            let filterColors = [
                [0.95, 0.4, 0.35, 0.8],
                [0.95, 0.65, 0.25, 0.8],
                [0.75, 0.85, 0.3, 0.8],
                [0.3, 0.8, 0.55, 0.8],
                [0.3, 0.7, 0.95, 0.8],
                [0.5, 0.45, 0.95, 0.8],
                [0.85, 0.4, 0.8, 0.8]
            ];
            let color = filterColors[(Number(curve.id) - 1) % filterColors.length];
            drawCurve(curve.values, curve.active === false
                ? [0.3, 0.3, 0.35, 0.5] : color);
        });
        let combined = presentation.combinedCurve;
        drawCurve(combined && combined.values,
            combined && combined.active === false
                ? [0.35, 0.35, 0.4, 0.6] : [0.2, 0.8, 1, 1]);
        (presentation.handles || []).forEach((handle) => {
            if (!handle.enabled) return;
            let preview = state && state.preview[handle.id];
            let frequency = preview ? preview.x : handle.frequency;
            let gain = preview ? preview.y : handle.gain;
            let x = layout.left + layout.width * frequency;
            let y = layout.top + layout.height * gain;
            graphics.set_source_rgba(handle.selected ? 1 : 0.95, 0.75, 0.25, 1);
            graphics.ellipse(x - 4, y - 4, 8, 8);
            graphics.fill();
        });
    }
}

module.exports = {
    AnalyzerRenderer: AnalyzerRenderer
};
