class AnalyzerPresentationBuilder
{
    begin(current, mode, enabled, parameterRevision, viewKey)
    {
        current = current || {};
        return {
            mode: String(mode),
            enabled: Number(enabled) !== 0,
            parameterRevision: Number(parameterRevision),
            viewKey: viewKey === undefined ? "" : String(viewKey),
            spectrum: current.spectrum || null,
            referenceSpectrum: current.referenceSpectrum || null,
            differenceSpectrum: current.differenceSpectrum || null,
            curves: (current.curves || []).slice(0),
            combinedCurve: current.combinedCurve || null,
            scopeActive: current.scopeActive || false,
            scopeColor: current.scopeColor || null,
            handles: []
        };
    }

    setCurve(presentation, name, args, id)
    {
        if (!presentation) return false;
        let curve = {
            active: Number(args[0]) !== 0,
            values: args.slice(1)
        };
        if (name === "spectrum") presentation.spectrum = curve;
        else if (name === "reference_spectrum") {
            presentation.referenceSpectrum = curve;
        }
        else if (name === "difference_spectrum") {
            presentation.differenceSpectrum = curve;
        }
        else if (name === "combined") presentation.combinedCurve = curve;
        else if (name === "curve") this.setFilterCurve(presentation, id, curve);
        else return false;
        return true;
    }

    setFilterCurve(presentation, id, curve)
    {
        let numericId = Number(id);
        for (let index = 0; index < presentation.curves.length; index += 1) {
            if (presentation.curves[index].id === numericId) {
                presentation.curves[index] = {
                    id: numericId,
                    active: curve.active,
                    values: curve.values
                };
                return;
            }
        }
        presentation.curves.push({
            id: numericId,
            active: curve.active,
            values: curve.values
        });
    }

    addHandle(presentation, args)
    {
        if (!presentation) return;
        let handle = {
            id: Number(args[0]),
            frequency: Number(args[1]),
            gain: Number(args[2]),
            enabled: Number(args[3]) !== 0,
            capabilities: {
                frequency: Number(args[4]) !== 0,
                gain: Number(args[5]) !== 0,
                q: Number(args[6]) !== 0
            },
            selected: Number(args[7]) !== 0,
            xMinimum: isFinite(Number(args[8])) ? Number(args[8]) : 0,
            xMaximum: isFinite(Number(args[9])) ? Number(args[9]) : 1,
            yMinimum: isFinite(Number(args[10])) ? Number(args[10]) : 0,
            yMaximum: isFinite(Number(args[11])) ? Number(args[11]) : 1
        };
        for (let index = 0; index < presentation.handles.length; index += 1) {
            if (presentation.handles[index].id === handle.id) {
                presentation.handles[index] = handle;
                return;
            }
        }
        presentation.handles.push(handle);
    }

    setScope(presentation, active, hasColor, red, green, blue, alpha)
    {
        if (!presentation) return;
        presentation.scopeActive = Number(active) !== 0;
        presentation.scopeColor = Number(hasColor) !== 0
            ? [Number(red), Number(green), Number(blue), Number(alpha)] : null;
    }
}

module.exports = {
    AnalyzerPresentationBuilder: AnalyzerPresentationBuilder
};
