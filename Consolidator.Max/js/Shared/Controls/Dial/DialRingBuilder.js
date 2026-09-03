const { normalizePresentationValue } = require("../../Presenters/Normalization.js");
const { presentationBindingValue } = require("../../Presenters/PresentationBinding.js");

class DialRingBuilder
{
    constructor(scope)
    {
        this.scope = scope;
    }

    build(configurations, maximumCount)
    {
        const rings = [];
        const mappings = [];
        (configurations || []).slice(0, maximumCount).forEach((configuration) => {
            const result = this.buildRing(configuration || {});
            rings.push(result.ring);
            mappings.push(result.mapping);
        });
        return { rings: rings, mappings: mappings };
    }

    buildRing(configuration)
    {
        const valueSource = configuration.value;
        const valueModel = valueSource && valueSource.value !== undefined
            ? valueSource
            : {};
        const physicalMinimum = this.readNumber(
            configuration.physicalMinimum,
            this.readNumber(valueModel.physicalMinimum, 0)
        );
        const physicalMaximum = this.readNumber(
            configuration.physicalMaximum,
            this.readNumber(valueModel.physicalMaximum, 1)
        );
        const logarithmic = (configuration.mapping || {}).type === "logarithmic";
        const groupScope = Boolean(this.scope && this.scope.isGroup());
        const minimum = this.readNumber(
            configuration.minimum,
            groupScope ? this.readNumber(valueModel.minimum, physicalMinimum) : physicalMinimum
        );
        const maximum = this.readNumber(
            configuration.maximum,
            groupScope ? this.readNumber(valueModel.maximum, physicalMaximum) : physicalMaximum
        );
        const value = this.readNumber(valueSource, physicalMinimum);
        const hasDefault = configuration.defaultValue !== undefined ||
            valueModel.defaultValue !== undefined;
        const defaultValue = hasDefault
            ? this.readNumber(
                configuration.defaultValue,
                this.readNumber(valueModel.defaultValue, value)
            )
            : null;

        return {
            ring: {
                value: normalizePresentationValue(
                    value,
                    physicalMinimum,
                    physicalMaximum,
                    logarithmic
                ),
                minimum: normalizePresentationValue(
                    minimum,
                    physicalMinimum,
                    physicalMaximum,
                    logarithmic
                ),
                maximum: normalizePresentationValue(
                    maximum,
                    physicalMinimum,
                    physicalMaximum,
                    logarithmic
                ),
                defaultValue: defaultValue === null
                    ? null
                    : normalizePresentationValue(
                        defaultValue,
                        physicalMinimum,
                        physicalMaximum,
                        logarithmic
                    ),
                visualization: this.buildVisualization(
                    presentationBindingValue(
                        configuration.visualization,
                        valueModel.visualization || null
                    )
                ),
                color: presentationBindingValue(
                    configuration.color,
                    valueModel.color || null
                )
            },
            mapping: {
                physicalMinimum: physicalMinimum,
                physicalMaximum: physicalMaximum,
                physicalStep: this.readNumber(
                    configuration.physicalStep,
                    this.readNumber(valueModel.physicalStep, 0)
                ),
                logarithmic: logarithmic
            }
        };
    }

    buildVisualization(source)
    {
        const visualization = source || {};
        const type = visualization.type || "none";
        if (type === "none")
        {
            return null;
        }
        const range = visualization.range || {};
        const minimum = this.readNumber(range.minimum, 0);
        const maximum = this.readNumber(range.maximum, 1);
        if (type === "level")
        {
            return {
                type: "level",
                peak: normalizePresentationValue(
                    this.readNumber(visualization.peak, minimum),
                    minimum,
                    maximum
                ),
                smoothed: normalizePresentationValue(
                    this.readNumber(visualization.smoothed, minimum),
                    minimum,
                    maximum
                )
            };
        }
        if (type === "reduction" || type === "saturation")
        {
            return {
                type: type,
                value: normalizePresentationValue(
                    this.readNumber(visualization.value, minimum),
                    minimum,
                    maximum
                )
            };
        }
        if (type === "relative")
        {
            return {
                type: "relative",
                value: Math.max(-1, Math.min(
                    1,
                    this.readNumber(visualization.value, 0)
                ))
            };
        }
        return null;
    }

    readNumber(source, fallback)
    {
        const value = presentationBindingValue(source, fallback);
        return typeof value === "number" && isFinite(value) ? value : fallback;
    }
}

module.exports = {
    DialRingBuilder: DialRingBuilder
};
