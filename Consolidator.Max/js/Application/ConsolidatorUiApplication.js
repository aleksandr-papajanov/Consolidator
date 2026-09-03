const { PresentationObservable } = require(
    "../Shared/Presenters/PresentationObservable.js");
const { ConsolidatorControlMapping } = require(
    "./ConsolidatorControlMapping.js");
const {
    createUiHostComponents,
    destroyUiHostComponents
} = require("./UiHostComposition.js");
const { installUiBindings } = require("./UiBindingInstaller.js");
const { parseTrackName } = require("./TrackNameParser.js");

class ConsolidatorUiHost
{
    constructor(source, sendNative, sendUi)
    {
        this.source = source;
        this.trackName = null;
        this.sendNative = sendNative || (() => {});
        this.sendUi = sendUi || (() => {});
        createUiHostComponents(this, source, this.sendNative);
        this.connectTargetLifecycle();
        this.registryInitialized = false;
        this.lifecycle = "created";
        this.instanceActive = false;
        this.publishedInstanceActive = null;
        this.metricsGestureActive = false;
        this.snapshotContext = "equalizer";
    }

    connectTargetLifecycle()
    {
        this.client.targetState.onTargetSnapshotBatchBegin(() => {
            PresentationObservable.beginBatch();
        });
        this.client.targetState.onTargetSnapshotBatchEnd(() => {
            PresentationObservable.endBatch();
        });
        this.client.targetState.onTargetTransitionBegin(() => {
            this.bindings.suspend();
        });
        this.client.targetState.onTargetTransitionDone(() => {
            this.bindings.resumeLatest();
        });
    }

    sendControlMessage(controlName, selector, args)
    {
        this.sendUi([controlName, selector].concat(args || []));
    }

    handleControl(selector, args)
    {
        if (selector === "instance_active") {
            if (args && args.length > 0) {
                this.setInstanceActive(Number(args[0]) !== 0);
            }
            return;
        }
        this.client.handleControl(selector, args);
    }

    setInstanceActive(active)
    {
        this.instanceActive = Boolean(active);
        if (this.instanceId !== undefined &&
                this.publishedInstanceActive === this.instanceActive) {
            return;
        }
        this.bankManagerViewModel.setRegistryActive(this.instanceActive);
        this.bindings.setPresentationActive(false);
        if (this.instanceId === undefined) return;

        this.publishedInstanceActive = this.instanceActive;
        this.client.setInstanceActive(this.instanceActive, (response) => {
            if (!response || response.error) {
                this.publishedInstanceActive = null;
                return;
            }
            if (!this.instanceActive) return;
            let target = this.client.targetState.target || {
                instanceId: this.instanceId,
                bankId: 0
            };
            this.client.uiTarget.show(
                target.instanceId,
                target.bankId,
                this.snapshotContext,
                (snapshotResponse) => {
                    if (this.instanceActive && snapshotResponse &&
                            !snapshotResponse.error) {
                        this.bindings.setPresentationActive(true);
                    }
                }
            );
        });
    }

    setTrackName(args)
    {
        this.trackName = parseTrackName(args);
        if (this.instanceId !== undefined) {
            this.client.state.set(
                "label", this.trackName, undefined, 0, "local");
        }
    }

    bind(varname, binding)
    {
        this.bindings.add(varname, binding((selector, args) => {
            this.sendControlMessage(varname, selector, args);
        }));
    }

    handleUiIntent(controlName, intent, values)
    {
        if (intent === "gestureBegan") {
            this.metricsGestureActive = true;
            this.sendMetrics();
        }
        this.bindings.handle(controlName, intent, values);
        if (intent === "gestureEnded") {
            this.metricsGestureActive = false;
            this.sendMetrics();
        }
        else if (intent !== "gestureBegan" && !this.metricsGestureActive) {
            this.sendMetrics();
        }
    }

    sendMetrics()
    {
        this.sendNative(["metrics"]);
    }

    bindControls()
    {
        installUiBindings(this);
    }

    initialize(mapping, callback)
    {
        if (this.lifecycle !== "created") return;
        this.lifecycle = "initializing";
        this.mapping = mapping || ConsolidatorControlMapping;
        this.bindControls();
        this.client.initialize((initialization) => {
            if (initialization.error) {
                this.lifecycle = "initialized";
                this.registryInitialized = false;
                if (callback) callback(initialization.error);
                return;
            }
            this.completeClientInitialization(initialization, callback);
        });
    }

    completeClientInitialization(initialization, callback)
    {
        this.instanceId = initialization.instanceId;
        this.snapshotContext = initialization.snapshotContext || "equalizer";
        this.bankManagerViewModel.setSelectedPanel(this.snapshotContext);
        this.viewModel.instanceId = this.instanceId;
        this.bankManager.context.instanceId = this.instanceId;
        this.bankManagerViewModel.setLocalInstanceId(this.instanceId);
        this.bankManagerViewModel.setFocusedBank(this.instanceId, 0);
        if (this.trackName !== null) {
            this.client.state.set(
                "label", this.trackName, undefined, 0, "local");
        }

        let finish = () => {
            this.setInstanceActive(this.instanceActive);
            this.registryInitialized = true;
            this.lifecycle = "initialized";
            if (callback) callback(null);
        };
        if (this.instanceActive) {
            this.bankManagerViewModel.setRegistryActive(
                true,
                (snapshot, response) => {
                    if (response && response.error) {
                        this.registryInitialized = false;
                        this.lifecycle = "initialized";
                        if (callback) callback(response.error);
                        return;
                    }
                    finish();
                }
            );
        }
        else {
            finish();
        }
        this.viewModel.initialize((error) => {
            if (error && callback) callback(error);
        });
    }

    undo(callback)
    {
        return this.client.transactions.undo(callback);
    }

    redo(callback)
    {
        return this.client.transactions.redo(callback);
    }

    destroy()
    {
        if (this.lifecycle === "destroyed") return;
        this.lifecycle = "destroyed";
        if (this.instanceId !== undefined && this.publishedInstanceActive) {
            this.client.setInstanceActive(false);
        }
        destroyUiHostComponents(this);
        this.metricsGestureActive = false;
        this.sendNative = () => {};
    }
}

module.exports = {
    ConsolidatorUiHost: ConsolidatorUiHost,
    ConsolidatorControlMapping: ConsolidatorControlMapping
};
