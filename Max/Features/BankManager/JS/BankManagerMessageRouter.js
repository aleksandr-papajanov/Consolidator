function BankManagerMessageRouter(manager) {
    this.manager = manager;
}

BankManagerMessageRouter.prototype.HandleSnapshot = function(values) {
    if (!values.length || String(values[0]) !== "snapshot") return;
    var store = String(values[3]);
    if (store === "eq") this.manager.ParseEqSnapshot(values);
    else if (store === "processor") this.manager.ParseProcessorSnapshot(values);
    else if (store === "device") this.manager.ParseDeviceSnapshot(values);
};

BankManagerMessageRouter.prototype.HandleLocal = function(name, values) {
    if (name === "eq_parameter_absolute_gesture") {
        this.manager.HandleEqAbsoluteParameterGesture(values);
    } else if (name === "eq_parameter_absolute_preview") {
        this.manager.HandleEqAbsoluteParameterPreview(values);
    } else if (name === "processor_parameter_gesture") {
        this.manager.HandleProcessorParameterGesture(values);
    } else if (name === "processor_match_operation") {
        this.manager.HandleProcessorMatchOperation(values);
    } else if (name === "processor_bypass_operation") {
        this.manager.HandleProcessorBypassOperation(values);
    } else if (name === "processor_detector_reset") {
        this.manager.HandleProcessorDetectorReset(values);
    } else if (name === "eq_filter_reset") {
        this.manager.ExecuteFilterReset(Number(values[0]), Number(values[1]));
    } else if (name === "bank.action") {
        this.manager.ExecuteOperation(String(values[0]), values[1]);
    }
};

BankManagerMessageRouter.prototype.Handle = function(inletIndex, name, values) {
    if (inletIndex === 1) {
        this.manager.HandleGlobal(name, values);
        return;
    }
    if (name === "snapshot") {
        this.HandleSnapshot(["snapshot"].concat(values));
        return;
    }
    this.HandleLocal(name, values);
};
