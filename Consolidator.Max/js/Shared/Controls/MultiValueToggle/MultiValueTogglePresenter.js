const { PresentationObservable } = require("../../Presenters/PresentationObservable.js");
const { presentationBindingSource } = require("../../Presenters/PresentationBinding.js");
const { presentationBindingValue } = require("../../Presenters/PresentationBinding.js");
const { presentationBindingWrite } = require("../../Presenters/PresentationBinding.js");
const { MultiValueTogglePresentation } = require("./MultiValueTogglePresentation.js");

class MultiValueTogglePresenter extends PresentationObservable
{
    constructor(options)
    {
        super();
        this.options = options || {};
        this.unsubscribers = [];
        let source = presentationBindingSource(this.options.value);
        if (source && typeof source.subscribe === "function") {
            this.unsubscribers.push(source.subscribe(() => this.rebuild(), false));
        }
        let scope = this.options.scope;
        if (scope && typeof scope.subscribe === "function") {
            this.unsubscribers.push(scope.subscribe(() => this.rebuild()));
        }
        this.rebuild();
    }

    rebuild()
    {
        let presentation = new MultiValueTogglePresentation();
        presentation.value = Math.max(0, Math.floor(Number(
            presentationBindingValue(this.options.value, 0)) || 0));
        presentation.values = (this.options.values || []).map(String);
        presentation.enabled = Boolean(presentationBindingValue(this.options.enabled, true));
        let scope = this.options.scope;
        presentation.scopeActive = Boolean(scope && typeof scope.isGroup === "function" && scope.isGroup());
        presentation.scopeColor = presentation.scopeActive ? scope.color : null;
        this.publish(presentation);
    }

    setValue(value, transactionId)
    {
        let count = (this.options.values || []).length;
        let next = Math.max(0, Math.min(count - 1, Math.floor(Number(value))));
        if (count > 0 && isFinite(next)) {
            presentationBindingWrite(this.options.value, next, transactionId);
        }
    }

    resetValue()
    {
        let source = presentationBindingSource(this.options.value);
        if (source && typeof source.reset === "function") source.reset();
    }

    destroy()
    {
        if (this.destroyed) return;
        this.unsubscribers.forEach((unsubscribe) => unsubscribe());
        this.unsubscribers = [];
        super.destroy();
    }
}

module.exports = { MultiValueTogglePresenter: MultiValueTogglePresenter };
