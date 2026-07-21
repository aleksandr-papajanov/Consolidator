autowatch = 1;
inlets = 1;
outlets = 0;

function SpectrumViewController() {
}

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

include('JS/ProcessorMetersConfig.js');
include('JS/SpectrumViewGeometry.js');
include('JS/SpectrumViewProcessorTelemetry.js');

var processorMetersController = new SpectrumViewController();

function paint() {
    try {
        var size = mgraphics.size;
        processorMetersController.DrawProcessorTelemetry(0, size[0], size[1]);
    } catch (error) {
        post("Processor meters paint error: " + error + "\n");
    }
}

function processor_telemetry() {
    processorMetersController.ProcessorTelemetry.apply(processorMetersController, arguments);
}
function inletassist() {
    assist("processor_telemetry <compressorReductionDb> <saturationNonlinearRatio> <saturationLevelDeltaDb>");
}
setinletassist(-1, inletassist);
