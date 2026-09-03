const {
    clampPresentationValue,
    denormalizePresentationValue
} = require("../../Presenters/Normalization.js");
const {
    presentationBindingSource,
    presentationBindingWrite
} = require("../../Presenters/PresentationBinding.js");

function writeDialValue(configuration, ring, mapping, normalizedValue, transactionId)
{
    if (!configuration || !ring || !mapping || !configuration.value)
    {
        return;
    }

    const clampedValue = clampPresentationValue(
        normalizedValue,
        ring.minimum,
        ring.maximum
    );
    let physicalValue = denormalizePresentationValue(
        clampedValue,
        mapping.physicalMinimum,
        mapping.physicalMaximum,
        mapping.logarithmic
    );
    const physicalStep = Number(mapping.physicalStep);
    if (isFinite(physicalStep) && physicalStep > 0)
    {
        physicalValue = mapping.physicalMinimum + Math.round(
            (physicalValue - mapping.physicalMinimum) / physicalStep
        ) * physicalStep;
    }
    const physicalMinimum = denormalizePresentationValue(
        ring.minimum,
        mapping.physicalMinimum,
        mapping.physicalMaximum,
        mapping.logarithmic
    );
    const physicalMaximum = denormalizePresentationValue(
        ring.maximum,
        mapping.physicalMinimum,
        mapping.physicalMaximum,
        mapping.logarithmic
    );
    physicalValue = clampPresentationValue(
        physicalValue,
        physicalMinimum,
        physicalMaximum
    );
    presentationBindingWrite(configuration.value, physicalValue, transactionId);
}

function resetDialValue(configuration, transactionId)
{
    const source = configuration && presentationBindingSource(configuration.value);
    if (source && typeof source.reset === "function")
    {
        source.reset(transactionId);
    }
}

module.exports = {
    resetDialValue: resetDialValue,
    writeDialValue: writeDialValue
};
