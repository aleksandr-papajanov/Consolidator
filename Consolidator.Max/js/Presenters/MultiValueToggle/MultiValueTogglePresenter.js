const { PresentationObservable } = require("../Core/PresentationObservable.js");
const { presentationBindingSource } = require("../Core/PresentationBinding.js");
const { presentationBindingValue } = require("../Core/PresentationBinding.js");
const { presentationBindingWrite } = require("../Core/PresentationBinding.js");
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

    setValue(value)
    {
        let count = (this.options.values || []).length;
        let next = Math.max(0, Math.min(count - 1, Math.floor(Number(value))));
        if (count > 0 && isFinite(next)) presentationBindingWrite(this.options.value, next);
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
