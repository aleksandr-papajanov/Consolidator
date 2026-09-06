const {
    clampPresentationValue,
    denormalizePresentationValue,
    normalizePresentationValue
} = require("../../Presenters/Normalization.js");
const {
    presentationBindingSource,
    presentationBindingWrite
} = require("../../Presenters/PresentationBinding.js");

function quantizeScalar(value)
{
    return Math.round(value / 0.1) * 0.1;
}

function quantizeDb(value)
{
    return Math.round(value / 3) * 3;
}

function quantizeRough(value, isDb)
{
    return isDb ? quantizeDb(value) : quantizeScalar(value);
}

function quantizeLog(value)
{
    let magnitude = Math.abs(value);
    if (magnitude === 0) return 0;
    let logarithmicStep = magnitude < 10
        ? 0.1
        : Math.pow(10, Math.floor(Math.log10(magnitude)));
    return Math.round(value / logarithmicStep) * logarithmicStep;
}

function writeDialValue(
    configuration,
    ring,
    mapping,
    normalizedValue,
    transactionId,
    precisionMode)
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
    if (precisionMode && precisionMode.isRough())
    {
        let isDb = mapping.unit === "dB" ||
            mapping.suffix === "dB";
        let roughStep = Number(mapping.roughStep);
        if (isFinite(roughStep) && roughStep > 0)
        {
            physicalValue = Math.round(physicalValue / roughStep) * roughStep;
        }
        else
        {
            physicalValue = quantizeRough(physicalValue, isDb);
        }
    }
    physicalValue = clampPresentationValue(
        physicalValue,
        physicalMinimum,
        physicalMaximum
    );
    presentationBindingWrite(configuration.value, physicalValue, transactionId);
    return normalizePresentationValue(
        physicalValue,
        physicalMinimum,
        physicalMaximum,
        mapping.logarithmic
    );
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
    writeDialValue: writeDialValue,
    quantizeRough: quantizeRough,
    quantizeLog: quantizeLog
};
