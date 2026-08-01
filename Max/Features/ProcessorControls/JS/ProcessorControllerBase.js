include("../../../Shared/Interface/Processor/ProcessorTelemetryViewModel.js");
include("../../../Shared/Interface/Processor/ProcessorTelemetryOptions.js");
include("../../../Shared/Interface/Processor/ValueCapture.js");
include("../../../Shared/Runtime/ControlControllerBase.js");
include("../../../Shared/Configuration/FilterDefinitions.js");

function ProcessorControllerBase(device) {
    ControlControllerBase.call(this, "processor.controls", this.FlushParameterUpdate, this);
    this.selectedBankId = 1;
    this.filterDefinitions = {};
    this.processorDefinitions = {};
    this.controlValues = {};
    this.visualDevice = device ? String(device) : "";
    this.processorBypassed = null;
    this.telemetry = new ProcessorTelemetryViewModel(
        this.ResetTelemetryIndicators,
        this
    );
    this.outputLevelIndicator = this.telemetry.LevelForProcessor(this.visualDevice);
    this.reductionIndicator = this.telemetry.reduction;
    this.saturationVisualization = this.telemetry.saturation;
    this.pendingProcessorLimits = {};
    this.linkColor = null;
    this.levelMatchCapture = new ValueCapture(
        "energyDb",
        0,
        null,
        null
    );
    this.onsetMatchCapture = new ValueCapture(
        "linear",
        0,
        null,
        null
    );
    this.matchStage = "idle";
    this.matchTask = new Task(this.CompleteMatch, this);
}

var ProcessorControlsController = ProcessorControllerBase;

ProcessorControllerBase.prototype = Object.create(ControlControllerBase.prototype);
ProcessorControllerBase.prototype.constructor = ProcessorControllerBase;

ProcessorControllerBase.prototype.Dispose = function() {
    ControlControllerBase.prototype.Dispose.call(this);
    this.matchTask.cancel();
    this.telemetry.Dispose();
    this.levelMatchCapture.Dispose();
    this.onsetMatchCapture.Dispose();
};

ProcessorControlsController.prototype.SendParameterGesture = function(
    device,
    parameter,
    normalized
) {
    outlet(3, "processor_parameter_gesture",
        String(device), String(parameter),
        Math.max(0, Math.min(1, Number(normalized))));
};

ProcessorControlsController.prototype.QueueParameterUpdate = function(
    device,
    parameter,
    normalized,
    commandName,
    commandFields,
    emitGesture
) {
    if (device === this.visualDevice &&
        (device === "compressor" || device === "saturator")) {
        this.ResetOutputLevelWindow();
    }
    this.parameterDispatcher.Enqueue(
        String(device) + ":" + String(parameter),
        {
            device: String(device),
            parameter: String(parameter),
            normalized: Math.max(0, Math.min(1, Number(normalized))),
            commandName: String(commandName),
            commandFields: commandFields,
            emitGesture: emitGesture !== false
        }
    );
};

ProcessorControlsController.prototype.FlushParameterUpdate = function(update) {
    if (update.emitGesture) {
        this.SendParameterGesture(
            update.device, update.parameter, update.normalized);
    }
    this.SendCommand(update.commandName, update.commandFields);
};

ProcessorControlsController.prototype.SendDetectorDefinitions = function() {
    var devices = ["compressor", "saturator"];
    for (var deviceIndex = 0; deviceIndex < devices.length; ++deviceIndex) {
        var device = devices[deviceIndex];
        var definition = this.processorDefinitions[device];
        if (!definition || device !== this.visualDevice) continue;
        var detectorDefinitions = FilterDefinitionCatalog.Detector();
        for (var filterId in detectorDefinitions) {
            if (!detectorDefinitions.hasOwnProperty(filterId)) continue;
            filterId = Number(filterId);
            var gain = this.FindParameter(definition, "detector." + filterId + ".gain");
            var frequency = this.FindParameter(
                definition, "detector." + filterId + ".frequency");
            var q = this.FindParameter(definition, "detector." + filterId + ".q");
            var bypass = this.FindParameter(
                definition, "detector." + filterId + ".bypass");
            if (!gain || !frequency || !q || !bypass) {
                outlet(2, "error", "missing_detector_definition", device, filterId,
                    !gain ? "gain" : !frequency ? "frequency" : !q ? "q" : "bypass");
                continue;
            }
            outlet(1, [
                "script", "sendbox", device + ".detectorCurve", "definition",
                filterId,
                gain.minimum, gain.maximum, gain.defaultValue,
                frequency.minimum, frequency.maximum, frequency.defaultValue,
                q.minimum, q.maximum, q.defaultValue,
                bypass.defaultValue
            ]);
        }
    }
};

ProcessorControlsController.prototype.DisplaySpec = function(parameter) {
    if (parameter === "attack" || parameter === "release") {
        return { decimals: 1, suffix: " ms" };
    }
    if (parameter === "mix" || parameter === "saturation") {
        return { decimals: 0, suffix: "%" };
    }
    if (parameter.indexOf("frequency") >= 0) {
        return { decimals: 0, suffix: " Hz" };
    }
    if (parameter.indexOf(".q") >= 0) return { decimals: 2, suffix: "" };
    if (parameter === "threshold" || parameter === "output" ||
        parameter.indexOf(".gain") >= 0) {
        return { decimals: 1, suffix: " dB" };
    }
    return { decimals: 2, suffix: "" };
};

ProcessorControlsController.prototype.SendDisplayRange = function(
    varName,
    ring,
    definition,
    parameterName,
    isSlider
) {
    if (!definition) return;
    var display = this.DisplaySpec(parameterName);
    var minimum = definition.minimum;
    var maximum = definition.maximum;
    if (parameterName === "mix" || parameterName === "saturation") {
        minimum *= 100;
        maximum *= 100;
    }
    var command = ["script", "sendbox", varName, "displayRange"];
    if (!isSlider) command.push(ring);
    command.push(minimum, maximum, definition.logarithmic ? 1 : 0,
        display.decimals, display.suffix);
    outlet(1, command);
    if (!isSlider) {
        outlet(1, [
            "script", "sendbox", varName, "defaultValue", ring,
            this.ToNormalized(definition, definition.defaultValue)
        ]);
    }
};

ProcessorControlsController.prototype.SendProcessorDisplayRanges = function() {
    var compressor = this.processorDefinitions.compressor;
    var saturator = this.processorDefinitions.saturator;
    if (compressor) {
        this.SendDisplayRange("compressor.thresholdOutput", 1,
            this.FindParameter(compressor, "threshold"), "threshold", false);
        this.SendDisplayRange("compressor.thresholdOutput", 2,
            this.FindParameter(compressor, "output"), "output", false);
        this.SendDisplayRange("compressor.attackRelease", 1,
            this.FindParameter(compressor, "attack"), "attack", false);
        this.SendDisplayRange("compressor.attackRelease", 2,
            this.FindParameter(compressor, "release"), "release", false);
        this.SendDisplayRange("compressor.mix", 0,
            this.FindParameter(compressor, "mix"), "mix", true);
    }
    if (saturator) {
        this.SendDisplayRange("saturator.saturationOutput", 1,
            this.FindParameter(saturator, "saturation"), "saturation", false);
        this.SendDisplayRange("saturator.saturationOutput", 2,
            this.FindParameter(saturator, "output"), "output", false);
    }
    this.SendTargetDisplayRanges();
};

ProcessorControlsController.prototype.SendTargetDisplayRanges = function() {
    var compressorTarget = ProcessorTelemetryOptions.targets.compressorReduction;
    var saturatorTarget = ProcessorTelemetryOptions.targets.saturatorPercent;
    var compressorDefault = compressorTarget.defaultValue /
        (compressorTarget.maximum - compressorTarget.minimum);
    var saturatorDefault = saturatorTarget.defaultValue /
        (saturatorTarget.maximum - saturatorTarget.minimum);
    if (this.visualDevice === "compressor") {
        this.SendTargetDisplayRange(
            "compressor.thresholdOutput",
            3,
            compressorTarget,
            compressorTarget.step,
            0,
            " dB");
        this.RememberValue(["compressor", "threshold-output", 3, compressorDefault]);
        outlet(1, ["script", "sendbox", "compressor.thresholdOutput", "outputValue", 3]);
    }
    if (this.visualDevice === "saturator") {
        this.SendTargetDisplayRange(
            "saturator.saturationOutput",
            3,
            saturatorTarget,
            saturatorTarget.step,
            0,
            "%");
        this.RememberValue(["saturator", "saturation-output", 3, saturatorDefault]);
        this.SendDialVisualization(
            3,
            "color",
            this.saturationVisualization.MapSensitivity(saturatorDefault)
        );
        outlet(1, ["script", "sendbox", "saturator.saturationOutput", "outputValue", 3]);
    }
};

ProcessorControlsController.prototype.SendTargetDisplayRange = function(
    varName,
    ring,
    target,
    step,
    decimals,
    suffix
) {
    outlet(1, [
        "script", "sendbox", varName, "displayRange", ring,
        target.minimum, target.maximum, 0, decimals, suffix
    ]);
    outlet(1, [
        "script", "sendbox", varName, "step", ring,
        step / (target.maximum - target.minimum)
    ]);
};

ProcessorControlsController.prototype.SendDialActivityEnabled = function(varName) {
    outlet(1, ["script", "sendbox", varName, "activityEnabled", 1]);
};

ProcessorControlsController.prototype.SendDialMatchEnabled = function(varName) {
    outlet(1, ["script", "sendbox", varName, "onsetMatchEnabled", 1]);
    outlet(1, ["script", "sendbox", varName, "levelMatchEnabled", 1]);
};

ProcessorControlsController.prototype.SendDialActive = function(varName, value) {
    outlet(1, ["script", "sendbox", varName, "active", value ? 1 : 0]);
};

ProcessorControlsController.prototype.ConfigureActivityControls = function() {
    if (this.visualDevice !== "compressor" && this.visualDevice !== "saturator") return;
    var primaryControl = this.visualDevice === "compressor"
        ? "compressor.thresholdOutput"
        : "saturator.saturationOutput";
    this.SendDialActivityEnabled(primaryControl);
    this.SendDialMatchEnabled(primaryControl);
};

ProcessorControlsController.prototype.FindParameter = function(definition, name) {
    if (!definition) return null;
    for (var index = 0; index < definition.parameters.length; index++) {
        if (definition.parameters[index].name === name) return definition.parameters[index];
    }
    return null;
};

ProcessorControlsController.prototype.IsDetectorId = function(filterId) {
    var detectors = FilterDefinitionCatalog.Detector();
    return detectors.hasOwnProperty(Number(filterId));
};

ProcessorControlsController.prototype.FindFilterParameter = function(definition, controlId) {
    if (controlId !== "frequency") return this.FindParameter(definition, controlId);
    return this.FindParameter(definition, "freq") || this.FindParameter(definition, "pivot");
};

ProcessorControlsController.prototype.FilterControlId = function(parameterName) {
    return parameterName === "freq" || parameterName === "pivot" ? "frequency" : parameterName;
};

ProcessorControlsController.prototype.ToAbsolute = function(definition, normalized) {
    if (!definition) return 0;
    var value = Math.max(0, Math.min(1, Number(normalized)));
    if (definition.logarithmic && definition.minimum > 0) {
        return definition.minimum * Math.pow(definition.maximum / definition.minimum, value);
    }
    return definition.minimum + value * (definition.maximum - definition.minimum);
};

ProcessorControlsController.prototype.ToNormalized = function(definition, absolute) {
    if (!definition) return 0;
    var value = Number(absolute);
    if (definition.logarithmic && definition.minimum > 0) {
        return Math.log(value / definition.minimum) / Math.log(definition.maximum / definition.minimum);
    }
    return (value - definition.minimum) / (definition.maximum - definition.minimum);
};

ProcessorControlsController.prototype.ResetDetector = function(device, filterId) {
    outlet(3, "processor_detector_reset", String(device), Number(filterId));
    outlet(1, ["script", "sendbox", device + ".detectorCurve", "reset",
        Number(filterId)]);
};

ProcessorControlsController.prototype.HandleProcessorLimits = function(device, parameter, minimum, maximum) {
    this.pendingProcessorLimits[device + ":" + parameter] = {
        device: device,
        parameter: parameter,
        minimum: minimum,
        maximum: maximum
    };
    this.ApplyProcessorLimits(device, parameter, minimum, maximum);
};

ProcessorControlsController.prototype.ApplyProcessorLimits = function(device, parameter, minimum, maximum) {
    if (String(device) !== this.visualDevice) return;
    var definition = this.FindParameter(this.processorDefinitions[device], String(parameter));
    if (!definition) return;
    var minimumNormalized = Math.max(0, Math.min(1, this.ToNormalized(definition, minimum)));
    var maximumNormalized = Math.max(0, Math.min(1, this.ToNormalized(definition, maximum)));
    if (parameter.indexOf("detector.") === 0) {
        var detectorParts = parameter.split(".");
        if (detectorParts.length === 3) {
            outlet(1, [
                "script", "sendbox", device + ".detectorCurve", "limits",
                Number(detectorParts[1]), String(detectorParts[2]),
                minimum, maximum
            ]);
        }
        return;
    }
    if (parameter === "threshold" || parameter === "saturation" || parameter === "output") {
        var primaryName = device === "compressor" ? "threshold" : "saturation";
        outlet(1, [
            "script", "sendbox", device + "." + primaryName + "Output", "limits",
            parameter === primaryName ? 1 : 2, minimumNormalized, maximumNormalized
        ]);
        return;
    } else if (device === "compressor" && (parameter === "attack" || parameter === "release")) {
        outlet(1, [
            "script", "sendbox", "compressor.attackRelease", "limits",
            parameter === "attack" ? 1 : 2, minimumNormalized, maximumNormalized
        ]);
        return;
    } else if (device === "compressor" && parameter === "mix") {
        outlet(1, [
            "script", "sendbox", "compressor.mix", "limits",
            minimumNormalized, maximumNormalized
        ]);
        return;
    }
};

ProcessorControlsController.prototype.HandleLocal = function(values) {
    if (values.length < 2) return;
    var device = String(values[0]);
    if (device === "filter") {
        var filterId = Number(values[1]);
        var parameterName = String(values[2]);
        if (parameterName === "bypass") {
            var bypassValue = Number(values[3]) ? 1 : 0;
            this.RememberValue(["filter", filterId, "bypass", bypassValue]);
            this.SendCommand("eq.set_bypass", [this.selectedBankId, filterId, bypassValue]);
            return;
        }
        var parameter = this.FindFilterParameter(this.filterDefinitions[filterId], parameterName);
        if (parameter) {
            var normalizedValue = Math.max(0, Math.min(1, Number(values[3])));
            this.RememberValue(["filter", filterId, parameterName, normalizedValue]);
            this.SendCommand("eq.set_parameter", [
                this.selectedBankId,
                filterId,
                parameter.name,
                this.ToAbsolute(parameter, normalizedValue)
            ]);
        }
        return;
    }

    if (device === "input_gain" || device === "output_gain") {
        var gainDefinition = this.processorDefinitions[device];
        var gainParameter = this.FindParameter(gainDefinition, "gain");
        if (!gainParameter) return;
        var gainValue = Math.max(0, Math.min(1, Number(values[1])));
        this.RememberValue([device, "gain", gainValue]);
        this.QueueParameterUpdate(
            device,
            "gain",
            gainValue,
            "gain.set_parameter",
            [
                device === "input_gain" ? "input" : "output",
                this.ToAbsolute(gainParameter, gainValue)
            ]);
        return;
    }

    if (device === "compressor" || device === "saturator") {
        var processorAction = String(values[1]);
        if (processorAction === "gesture") {
            this.HandleHistoryGesture(values[2]);
            return;
        }
        if (String(values[2]) === "gesture") {
            this.HandleHistoryGesture(values[3]);
            return;
        }
        if (processorAction === "detector_absolute" && String(values[3]) === "bypass") {
            var bypassDetectorId = Number(values[2]);
            if (!this.IsDetectorId(bypassDetectorId)) return;
            var bypassDefinition = this.FindParameter(
                this.processorDefinitions[device],
                "detector." + bypassDetectorId + ".bypass"
            );
            if (!bypassDefinition) return;
            var bypassValue = Number(values[4]) ? 1 : 0;
            this.QueueParameterUpdate(
                device,
                "detector." + bypassDetectorId + ".bypass",
                bypassValue,
                device + ".set_detector_parameter",
                [bypassDetectorId, "bypass", bypassValue]
            );
            return;
        }
        if (processorAction === "detector_absolute" && String(values[3]) === "reset") {
            var resetDetectorId = Number(values[2]);
            if (!this.IsDetectorId(resetDetectorId)) return;
            this.ResetDetector(device, resetDetectorId);
            return;
        }
        if (processorAction === "detector_absolute" && values.length >= 5) {
            var absoluteDetectorId = Number(values[2]);
            var absoluteDetectorParameter = String(values[3]);
            var absoluteDetectorValue = Number(values[4]);
            if (!this.IsDetectorId(absoluteDetectorId)) return;
            if (["gain", "frequency", "q"].indexOf(absoluteDetectorParameter) < 0) return;
            var absoluteDefinition = this.FindParameter(
                this.processorDefinitions[device],
                "detector." + absoluteDetectorId + "." + absoluteDetectorParameter
            );
            if (!absoluteDefinition) return;
            var normalizedDetectorValue = this.ToNormalized(
                absoluteDefinition,
                absoluteDetectorValue
            );
            this.QueueParameterUpdate(
                device,
                "detector." + absoluteDetectorId + "." + absoluteDetectorParameter,
                normalizedDetectorValue,
                device + ".set_detector_parameter",
                [absoluteDetectorId, absoluteDetectorParameter, absoluteDetectorValue]
            );
            return;
        }
        if (processorAction === "detector_listen") {
            var listenFilterId = Number(values[2]);
            var listenEnabled = Number(values[3]) !== 0;
            if (!this.IsDetectorId(listenFilterId)) return;
            this.SendCommand(device + ".set_detector_listen", [
                listenFilterId, listenEnabled ? 1 : 0
            ]);
            return;
        }
        if ((processorAction === "threshold-output" ||
            processorAction === "saturation-output") &&
            (String(values[2]) === "onsetMatch" ||
                String(values[2]) === "levelMatch")) {
            this.RequestMatch(device, String(values[2]) === "onsetMatch"
                ? "onset" : "level", values[3]);
            return;
        }
        if ((processorAction === "threshold-output" ||
            processorAction === "saturation-output") &&
            String(values[2]) === "active") {
            var processorActive = Number(values[3]) !== 0;
            outlet(3, "processor_bypass_operation", device,
                processorActive ? 0 : 1);
            return;
        }
        if ((processorAction === "threshold-output" ||
            processorAction === "saturation-output") && values.length >= 4) {
            var primaryOutputIndex = Number(values[2]);
            var primaryOutputValue = Math.max(0, Math.min(1, Number(values[3])));
            if (primaryOutputIndex === 3) {
                this.RememberValue([
                    device,
                    processorAction,
                    primaryOutputIndex,
                    primaryOutputValue
                ]);
                if (device === "saturator") {
                    this.SendDialVisualization(
                        3,
                        "color",
                        this.saturationVisualization.MapSensitivity(primaryOutputValue)
                    );
                }
                return;
            }
            if (primaryOutputIndex !== 1 && primaryOutputIndex !== 2) return;
            var primaryParameter = device === "compressor" ? "threshold" : "saturation";
            var primaryOutputParameter = primaryOutputIndex === 1 ? primaryParameter : "output";
            var primaryOutputDefinition = this.FindParameter(
                this.processorDefinitions[device], primaryOutputParameter);
            if (!primaryOutputDefinition) {
                outlet(2, "error", "missing_processor_definition", device, primaryOutputParameter);
                return;
            }
            this.RememberValue([
                device,
                processorAction,
                primaryOutputIndex,
                primaryOutputValue
            ]);
            this.QueueParameterUpdate(
                device,
                primaryOutputParameter,
                primaryOutputValue,
                device + ".set_parameter",
                [
                    primaryOutputParameter,
                    this.ToAbsolute(primaryOutputDefinition, primaryOutputValue)
                ]);
            return;
        }
        if (processorAction === "attack-release" && values.length >= 4) {
            var attackReleaseIndex = Number(values[2]);
            var attackReleaseValue = Math.max(0, Math.min(1, Number(values[3])));
            if (device !== "compressor" || (attackReleaseIndex !== 1 && attackReleaseIndex !== 2)) return;
            var attackReleaseParameter = attackReleaseIndex === 1 ? "attack" : "release";
            var attackReleaseDefinition = this.FindParameter(this.processorDefinitions.compressor, attackReleaseParameter);
            if (!attackReleaseDefinition) return;
            this.QueueParameterUpdate(
                "compressor",
                attackReleaseParameter,
                attackReleaseValue,
                "compressor.set_parameter",
                [
                    attackReleaseParameter,
                    this.ToAbsolute(attackReleaseDefinition, attackReleaseValue)
                ]);
            return;
        }
    }

    var name = String(values[1]);
    var definition = this.processorDefinitions[device];
    var processorParameter = this.FindParameter(definition, name);
    if (!processorParameter) {
        outlet(2, "error", "missing_processor_definition", device, name);
        return;
    }
    var processorValue = Math.max(0, Math.min(1, Number(values[2])));
    this.RememberValue([device, name, processorValue]);
    var absolute = this.ToAbsolute(processorParameter, processorValue);
    this.QueueParameterUpdate(
        device,
        name,
        processorValue,
        device + ".set_parameter",
        [name, absolute]);
};

ProcessorControlsController.prototype.MatchOutputLevel = function(device, measuredDb) {
    if (device !== this.visualDevice || this.processorBypassed) return;
    var definition = this.FindParameter(this.processorDefinitions[device], "output");
    if (!definition || measuredDb === null) return;
    var controlName = device === "compressor"
        ? "threshold-output"
        : "saturation-output";
    var stateKey = this.ControlStateKey([device, controlName, 2, 0]);
    var currentNormalized = this.controlValues[stateKey];
    if (currentNormalized === undefined) return;
    var targetDb = this.outputLevelIndicator.targetDb;
    var currentDb = this.ToAbsolute(definition, currentNormalized);
    var nextDb = Math.max(
        definition.minimum,
        Math.min(definition.maximum, currentDb + targetDb - measuredDb)
    );
    var nextNormalized = this.ToNormalized(definition, nextDb);
    this.SendValue([device, controlName, 2, nextNormalized]);
    this.QueueParameterUpdate(
        device,
        "output",
        nextNormalized,
        device + ".set_parameter",
        ["output", nextDb],
        false
    );
};

ProcessorControlsController.prototype.GetTargetAbsolute = function(
    device,
    action,
    ring,
    target
) {
    var normalized = this.controlValues[
        this.ControlStateKey([device, action, ring, 0])
    ];
    if (normalized === undefined) return target.defaultValue;
    return target.minimum + normalized * (target.maximum - target.minimum);
};

ProcessorControlsController.prototype.MatchControlName = function() {
    return this.visualDevice === "compressor"
        ? "compressor.thresholdOutput"
        : this.visualDevice === "saturator"
            ? "saturator.saturationOutput"
            : "";
};

ProcessorControlsController.prototype.SetMatchActive = function(stage, value) {
    var varName = this.MatchControlName();
    if (!varName) return;
    outlet(1, ["script", "sendbox", varName,
        stage === "onset" ? "onsetMatch" : "levelMatch", value ? 1 : 0]);
};

ProcessorControlsController.prototype.SetMatchDialEnabled = function(value) {
    var varName = this.MatchControlName();
    if (varName) outlet(1, ["script", "sendbox", varName, "enabled", value ? 1 : 0]);
};

ProcessorControlsController.prototype.BeginMatch = function(stage) {
    if (this.matchStage !== "idle") return;
    var capture = stage === "onset"
        ? this.onsetMatchCapture : this.levelMatchCapture;
    capture.Begin();
    this.matchStage = stage;
    this.matchTask.cancel();
    this.matchTask.schedule(
        ProcessorTelemetryOptions.capture.maximumDurationMilliseconds
    );
    this.SetMatchDialEnabled(0);
    this.SetMatchActive(stage, 1);
};

ProcessorControlsController.prototype.FinishMatch = function() {
    var stage = this.matchStage;
    this.matchTask.cancel();
    this.matchStage = "idle";
    this.SetMatchDialEnabled(1);
    if (stage !== "idle") this.SetMatchActive(stage, 0);
};

ProcessorControlsController.prototype.CompleteMatch = function() {
    if (this.matchStage === "onset") {
        this.MatchOnset(this.visualDevice, this.onsetMatchCapture.Finish());
        this.FinishMatch();
        return;
    }
    if (this.matchStage === "level") {
        this.MatchOutputLevel(this.visualDevice, this.levelMatchCapture.Finish());
        this.FinishMatch();
    }
};

ProcessorControlsController.prototype.RequestMatch = function(device, operation, value) {
    if (device !== this.visualDevice) return;
    if (Number(value) === 0) {
        this.CompleteMatch();
        return;
    }
    outlet(3, "processor_match_operation", device, operation);
};

ProcessorControlsController.prototype.HandleGroupMatch = function(device, operation) {
    if (device !== this.visualDevice ||
        (operation !== "onset" && operation !== "level")) return;
    this.BeginMatch(operation);
};

ProcessorControlsController.prototype.HandleGroupBypass = function(device, bypass) {
    if (device !== this.visualDevice) return;
    this.processorBypassed = Number(bypass) !== 0;
    this.SendDialActive(this.MatchControlName(), !this.processorBypassed);
    if (!this.processorBypassed) return;
    this.telemetry.Reset();
    this.CancelMatchCaptures();
};

ProcessorControlsController.prototype.CancelMatchCaptures = function() {
    this.levelMatchCapture.Cancel();
    this.onsetMatchCapture.Cancel();
    this.matchTask.cancel();
    this.matchStage = "idle";
    this.SetMatchDialEnabled(1);
    this.SetMatchActive("onset", 0);
    this.SetMatchActive("level", 0);
};

ProcessorControlsController.prototype.MatchOnset = function(device, measuredValue) {
    if (device !== this.visualDevice || this.processorBypassed) return;
    if (measuredValue === null) return;
    var definition;
    var parameter;
    var action;
    var currentNormalized;
    var nextNormalized;
    if (device === "compressor") {
        if (!this.reductionIndicator.HasNormalizedValue()) return;
        definition = this.FindParameter(this.processorDefinitions.compressor, "threshold");
        action = "threshold-output";
        currentNormalized = this.controlValues[
            this.ControlStateKey([device, action, 1, 0])
        ];
        if (!definition || currentNormalized === undefined) return;
        var reductionDb = Number(measuredValue);
        var thresholdDb = this.ToAbsolute(definition, currentNormalized);
        var targetReductionDb = this.GetTargetAbsolute(
            "compressor", "threshold-output", 3,
            ProcessorTelemetryOptions.targets.compressorReduction);
        var reductionSlope = ProcessorTelemetryOptions.onsetMatch
            .compressorReductionPerThresholdDb;
        var nextThresholdDb = Math.max(
            definition.minimum,
            Math.min(
                definition.maximum,
                thresholdDb + (reductionDb - targetReductionDb) / reductionSlope
            )
        );
        nextNormalized = this.ToNormalized(definition, nextThresholdDb);
        parameter = "threshold";
    } else if (device === "saturator") {
        definition = this.FindParameter(this.processorDefinitions.saturator, "saturation");
        action = "saturation-output";
        currentNormalized = this.controlValues[
            this.ControlStateKey([device, action, 1, 0])
        ];
        var nonlinearRatio = Number(measuredValue);
        if (!definition || currentNormalized === undefined || nonlinearRatio === null) return;
        var targetPercent = this.GetTargetAbsolute(
            "saturator", "saturation-output", 3,
            ProcessorTelemetryOptions.targets.saturatorPercent);
        var targetRatio = targetPercent * 0.01;
        if (nonlinearRatio <= ProcessorTelemetryOptions.onsetMatch.minimumSaturatorRatio) {
            nextNormalized = Math.max(
                currentNormalized,
                ProcessorTelemetryOptions.onsetMatch.initialSaturation
            );
        } else {
            nextNormalized = currentNormalized * Math.sqrt(targetRatio / nonlinearRatio);
        }
        nextNormalized = Math.max(0.0, Math.min(1.0, nextNormalized));
        parameter = "saturation";
    } else {
        return;
    }
    this.SendValue([device, action, 1, nextNormalized]);
    this.QueueParameterUpdate(
        device,
        parameter,
        nextNormalized,
        device + ".set_parameter",
        [parameter, this.ToAbsolute(definition, nextNormalized)],
        false
    );
};

ProcessorControlsController.prototype.ControlVarName = function(fields) {
    if (String(fields[1]) === "threshold-output") return "compressor.thresholdOutput";
    if (String(fields[1]) === "saturation-output") return "saturator.saturationOutput";
    if (String(fields[1]) === "attack-release") return String(fields[0]) + ".attackRelease";
    if (String(fields[1]) === "detector") {
        return String(fields[0]) + ".detector." + Number(fields[2]);
    }
    if (String(fields[0]) === "filter") {
        return "filter." + Number(fields[1]) + "." + String(fields[2]);
    }
    return String(fields[0]) + "." + String(fields[1]);
};

ProcessorControlsController.prototype.RememberValue = function(fields) {
    this.controlValues[this.ControlStateKey(fields)] = Number(fields[fields.length - 1]);
};

ProcessorControlsController.prototype.ControlStateKey = function(fields) {
    var varName = this.ControlVarName(fields);
    if (String(fields[1]) === "threshold-output" ||
        String(fields[1]) === "saturation-output" ||
        String(fields[1]) === "attack-release") {
        return varName + "." + Number(fields[2]);
    }
    if (String(fields[1]) === "detector") {
        var detectorParameter = String(fields[3]);
        var detectorRing = { gain: 1, frequency: 2, q: 3 }[detectorParameter];
        return varName + "." + (detectorRing || detectorParameter);
    }
    return varName;
};

ProcessorControlsController.prototype.SendValue = function(fields) {
    var value = fields[fields.length - 1];
    var varName = this.ControlVarName(fields);
    var previous = this.controlValues[this.ControlStateKey(fields)];
    if (previous !== undefined && Math.abs(previous - value) <= 0.0000001) return;
    this.controlValues[this.ControlStateKey(fields)] = value;
    if (String(fields[1]) === "mix") {
        outlet(1, ["script", "sendbox", varName, "setValue", value]);
        return;
    }
    if (String(fields[1]) === "detector") {
        if (String(fields[3]) === "bypass") {
            outlet(1, ["script", "sendbox", varName, "active", Number(value) ? 0 : 1]);
            return;
        }
        var ringIndex = { gain: 1, frequency: 2, q: 3 }[String(fields[3])];
        if (ringIndex) {
            outlet(1, ["script", "sendbox", varName, "set", ringIndex, value]);
            return;
        }
    }
    if (String(fields[1]) === "threshold-output" ||
        String(fields[1]) === "saturation-output" ||
        String(fields[1]) === "attack-release") {
        outlet(1, ["script", "sendbox", varName, "set", Number(fields[2]), value]);
        return;
    }
    outlet(1, ["script", "sendbox", varName, "set", value]);
};

ProcessorControlsController.prototype.SendDetectorListen = function(device, filterId, enabled) {
    if (device !== this.visualDevice) return;
    outlet(1, ["script", "sendbox", device + ".detectorCurve",
        "listen", Number(filterId), enabled ? 1 : 0]);
};

ProcessorControlsController.prototype.HandleEqSnapshot = function(values) {
    this.selectedBankId = Number(values[5]);
    var chainBypass = Number(values[6]);
    var position = 9;
    var bankCount = Number(values[8]);
    this.SendValue(["eq", "bypass", chainBypass]);
    for (var bankIndex = 0; bankIndex < bankCount; bankIndex++) {
        var bankId = Number(values[position++]);
        position++;
        var filterCount = Number(values[position++]);
        for (var filterIndex = 0; filterIndex < filterCount; filterIndex++) {
            var filterId = Number(values[position++]);
            var bypass = Number(values[position++]);
            var valueCount = Number(values[position++]);
            var definition = this.filterDefinitions[filterId];
            if (bankId === this.selectedBankId && definition) {
                for (var valueIndex = 0; valueIndex < valueCount; valueIndex++) {
                    var parameter = definition.parameters[valueIndex];
                    this.SendValue([
                        "filter", filterId, this.FilterControlId(parameter.name),
                        this.ToNormalized(parameter, values[position + valueIndex])
                    ]);
                }
                this.SendValue(["filter", filterId, "bypass", bypass]);
            }
            position += valueCount;
        }
    }
};

ProcessorControlsController.prototype.HandleProcessorSnapshot = function(values) {
    var count = values.length;
    if (count < 29 || !this.processorDefinitions.input_gain ||
        !this.processorDefinitions.compressor || !this.processorDefinitions.saturator ||
        !this.processorDefinitions.output_gain) return;
    var inputGain = this.processorDefinitions.input_gain;
    var compressor = this.processorDefinitions.compressor;
    var saturator = this.processorDefinitions.saturator;
    var outputGain = this.processorDefinitions.output_gain;
    var base = count - 29;
    var inputGainNormalized = this.ToNormalized(this.FindParameter(inputGain, "gain"), values[base]);
    var outputGainNormalized = this.ToNormalized(this.FindParameter(outputGain, "gain"), values[base + 28]);
    this.SendValue(["input_gain", "gain", inputGainNormalized]);
    var compressorBypass = Number(values[base + 1]);
    var deviceBypass = this.visualDevice === "compressor"
        ? compressorBypass !== 0
        : Number(values[base + 16]) !== 0;
    if (this.processorBypassed !== deviceBypass && deviceBypass) {
        this.telemetry.Reset();
        this.CancelMatchCaptures();
    }
    this.processorBypassed = deviceBypass;
    this.SendValue(["compressor", "attack-release", 1, this.ToNormalized(this.FindParameter(compressor, "attack"), values[base + 2])]);
    this.SendValue(["compressor", "attack-release", 2, this.ToNormalized(this.FindParameter(compressor, "release"), values[base + 3])]);
    this.SendValue(["compressor", "threshold-output", 1, this.ToNormalized(this.FindParameter(compressor, "threshold"), values[base + 4])]);
    this.SendValue(["compressor", "threshold-output", 2, this.ToNormalized(this.FindParameter(compressor, "output"), values[base + 5])]);
    this.SendValue(["compressor", "mix", Number(values[base + 6])]);
    this.SendDetectorSnapshot("compressor", values, base + 7);
    var compressorListen = Number(values[base + 15]);
    var saturatorBypass = Number(values[base + 16]);
    this.SendValue(["saturator", "saturation-output", 1,
        this.ToNormalized(this.FindParameter(saturator, "saturation"), values[base + 17])]);
    this.SendValue(["saturator", "saturation-output", 2,
        this.ToNormalized(this.FindParameter(saturator, "output"), values[base + 18])]);
    this.SendDetectorSnapshot("saturator", values, base + 19);
    var saturatorListen = Number(values[base + 27]);
    for (var detectorId = 1; detectorId <= 2; detectorId++) {
        this.SendDetectorListen(
            "compressor", detectorId,
            (compressorListen & (1 << (detectorId - 1))) !== 0 &&
                Number(values[base + (detectorId === 1 ? 7 : 11)]) === 0);
        this.SendDetectorListen(
            "saturator", detectorId,
            (saturatorListen & (1 << (detectorId - 1))) !== 0 &&
                Number(values[base + (detectorId === 1 ? 19 : 23)]) === 0);
    }
    this.SendDialActive("compressor.thresholdOutput", !compressorBypass);
    this.SendDialActive("saturator.saturationOutput", !saturatorBypass);
    this.SendValue(["output_gain", "gain", outputGainNormalized]);
};

ProcessorControlsController.prototype.SendDialLinkColor = function(varName, ringCount) {
    for (var ring = 1; ring <= ringCount; ring++) {
        if (this.linkColor) {
            outlet(1, ["script", "sendbox", varName, "ringColor", ring].concat(this.linkColor));
        } else {
            outlet(1, ["script", "sendbox", varName, "clearRingColor", ring]);
        }
    }
};

ProcessorControlsController.prototype.HandleLinkColor = function(linkId, red, green, blue, alpha) {
    this.linkColor = String(linkId) === "-"
        ? null
        : [Number(red), Number(green), Number(blue), Number(alpha)];
    var device = this.visualDevice;
    if (device !== "compressor" && device !== "saturator") return;
    outlet(1, ["script", "sendbox", device + ".detectorCurve", "link_color",
        String(linkId), Number(red), Number(green), Number(blue), Number(alpha)]);
    if (device === "compressor") {
        this.SendDialLinkColor("compressor.thresholdOutput", 2);
        this.SendDialLinkColor("compressor.attackRelease", 2);
        if (this.linkColor) {
            outlet(1, ["script", "sendbox", "compressor.mix", "valueColor"]
                .concat(this.linkColor));
        } else {
            outlet(1, ["script", "sendbox", "compressor.mix", "clearValueColor"]);
        }
    } else {
        this.SendDialLinkColor("saturator.saturationOutput", 2);
    }
};

ProcessorControlsController.prototype.SendDetectorSnapshot = function(device, values, position) {
    for (var index = 0; index < 2; index++) {
        var base = position + index * 4;
        this.SendDetectorCurve(
            device,
            index + 1,
            values[base],
            values[base + 1],
            values[base + 2],
            values[base + 3]
        );
    }
};

ProcessorControlsController.prototype.SendDetectorCurve = function(
    device,
    filterId,
    bypass,
    gainDb,
    frequencyHz,
    q
) {
    if (device !== this.visualDevice) return;
    outlet(1, [
        "script",
        "sendbox",
        device + ".detectorCurve",
        "detector",
        filterId,
        Number(bypass) ? 1 : 0,
        Number(gainDb),
        Number(frequencyHz),
        Number(q)
    ]);
};

ProcessorControlsController.prototype.HandleSnapshot = function(values) {
    if (values.length < 6 || String(values[0]) !== "snapshot") return;
    var store = String(values[3]);
    if (store === "eq") this.HandleEqSnapshot(values);
    else if (store === "processor") this.HandleProcessorSnapshot(values);
};

ProcessorControlsController.prototype.HandleProcessorPreview = function(
    device,
    parameter,
    absoluteValue
) {
    device = String(device);
    parameter = String(parameter);
    if (device !== this.visualDevice) return;
    var definition = this.FindParameter(this.processorDefinitions[device], parameter);
    if (!definition) return;
    if (parameter.indexOf("detector.") === 0) {
        var detectorParts = parameter.split(".");
        if (detectorParts.length === 3) {
            outlet(1, [
                "script", "sendbox", device + ".detectorCurve", "preview",
                Number(detectorParts[1]), String(detectorParts[2]),
                Number(absoluteValue)
            ]);
        }
        return;
    }
    var value = this.ToNormalized(definition, Number(absoluteValue));
    this.ResetOutputLevelWindow();
    if (device === "compressor") {
        if (parameter === "threshold") this.SendValue(["compressor", "threshold-output", 1, value]);
        else if (parameter === "output") this.SendValue(["compressor", "threshold-output", 2, value]);
        else if (parameter === "attack") this.SendValue(["compressor", "attack-release", 1, value]);
        else if (parameter === "release") this.SendValue(["compressor", "attack-release", 2, value]);
        else if (parameter === "mix") this.SendValue(["compressor", "mix", value]);
    } else if (device === "saturator") {
        if (parameter === "saturation") this.SendValue(["saturator", "saturation-output", 1, value]);
        else if (parameter === "output") this.SendValue(["saturator", "saturation-output", 2, value]);
    }
};

ProcessorControlsController.prototype.SendOutputLevelIndicator = function() {
    if (this.visualDevice !== "compressor" && this.visualDevice !== "saturator") return;
    var display = this.telemetry.DisplayProcessorLevel(this.visualDevice);
    this.SendDialLevelVisualization(
        2,
        display.peak,
        display.smoothed
    );
};

ProcessorControlsController.prototype.ResetOutputLevelWindow = function() {
    if (this.visualDevice !== "compressor" && this.visualDevice !== "saturator") return;
    this.outputLevelIndicator.Reset();
    this.SendOutputLevelIndicator();
};

ProcessorControlsController.prototype.ResetTelemetryIndicators = function() {
    this.SendOutputLevelIndicator();
    if (this.visualDevice === "compressor") {
        var reduction = this.telemetry.DisplayReduction();
        this.SendDialReductionVisualization(2, reduction);
        this.SendDialVisualization(2, "relative", reduction);
    } else if (this.visualDevice === "saturator") {
        this.SendDialVisualization(1, "color", this.telemetry.DisplaySaturation());
    }
};

ProcessorControlsController.prototype.Initialize = function() {
    this.filterDefinitions = FilterDefinitionCatalog.Eq();
    this.processorDefinitions = FilterDefinitionCatalog.Processors();
    this.telemetry.Reset();
    this.SendProcessorDisplayRanges();
    this.SendDetectorDefinitions();
    this.ConfigureActivityControls();
    this.HandleLinkColor(
        this.linkColor ? "active" : "-",
        this.linkColor ? this.linkColor[0] : 0,
        this.linkColor ? this.linkColor[1] : 0,
        this.linkColor ? this.linkColor[2] : 0,
        this.linkColor ? this.linkColor[3] : 0);
    for (var key in this.pendingProcessorLimits) {
        if (!this.pendingProcessorLimits.hasOwnProperty(key)) continue;
        var limit = this.pendingProcessorLimits[key];
        this.ApplyProcessorLimits(
            limit.device, limit.parameter, limit.minimum, limit.maximum);
    }
};

ProcessorControlsController.prototype.SendDialVisualization = function(ring, mode, value) {
    if (this.visualDevice !== "compressor" && this.visualDevice !== "saturator") return;
    outlet(1, [
        "script",
        "sendbox",
        this.visualDevice === "compressor"
            ? "compressor.thresholdOutput"
            : "saturator.saturationOutput",
        "visualization",
        ring,
        mode,
        value
    ]);
};

ProcessorControlsController.prototype.SendDialReductionVisualization = function(
    ring,
    value
) {
    if (this.visualDevice !== "compressor") return;
    outlet(1, [
        "script", "sendbox", "compressor.thresholdOutput",
        "reductionVisualization", ring, value
    ]);
};

ProcessorControlsController.prototype.SendDialLevelVisualization = function(
    ring,
    peakValue,
    smoothedValue
) {
    if (this.visualDevice !== "compressor" && this.visualDevice !== "saturator") return;
    outlet(1, [
        "script", "sendbox",
        this.visualDevice === "compressor"
            ? "compressor.thresholdOutput"
            : "saturator.saturationOutput",
        "levelVisualization", ring, peakValue, smoothedValue
    ]);
};

ProcessorControlsController.prototype.HandleTargetLevel = function(value) {
    this.outputLevelIndicator.SetTargetDb(value);
};

ProcessorControlsController.prototype.HandleProcessorTelemetry = function(values) {
    if (this.processorBypassed) return;
    if (!this.telemetry.Update(values)) return;
    if (this.matchStage === "level" && this.levelMatchCapture.active) {
        this.levelMatchCapture.Observe(this.outputLevelIndicator.RawLevelDb());
    }
    if (this.matchStage === "onset" && this.onsetMatchCapture.active) {
        this.onsetMatchCapture.Observe(this.visualDevice === "compressor"
            ? Math.max(0.0, -Number(values[0]))
            : this.saturationVisualization.rawValue);
    }
    this.SendOutputLevelIndicator();
    if (this.visualDevice === "compressor") {
        var reduction = this.telemetry.DisplayReduction();
        this.SendDialReductionVisualization(
            2,
            reduction
        );
    } else {
        this.SendDialVisualization(1, "color", this.telemetry.DisplaySaturation());
    }
};
