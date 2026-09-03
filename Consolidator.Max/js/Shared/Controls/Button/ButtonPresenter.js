const { PresentationObservable } = require("../../Presenters/PresentationObservable.js");
const { presentationBindingSource } = require("../../Presenters/PresentationBinding.js");
const { presentationBindingValue } = require("../../Presenters/PresentationBinding.js");
const { subscribePresentationBinding } = require("../../Presenters/PresentationBinding.js");
const { presentationBindingWrite } = require("../../Presenters/PresentationBinding.js");
const { ButtonPresentation } = require("./ButtonPresentation.js");

class ButtonPresenter extends PresentationObservable
{
    constructor(options)
    {
        super();
        this.options = options || {};
        this.unsubscribers = [];
        this.subscribeSources();
        this.rebuild();
    }
    
    read(source, fallback)
    {
        return presentationBindingValue(source, fallback);
    }
    
    subscribeSources()
    {
        let sources = [this.options.value, this.options.enabled,
            this.options.loading, this.options.active, this.options.scope];
        let subscribedSources = [];
        for (let index = 0; index < sources.length; index += 1) {
            let source = presentationBindingSource(sources[index]);
            if (!source || subscribedSources.indexOf(source) >= 0) {
                continue;
            }
            subscribedSources.push(source);
            subscribePresentationBinding(sources[index], () => {
                this.requestRebuild();
            }, this.unsubscribers);
        }
    }
    
    rebuild()
    {
        let presentation = new ButtonPresentation();
        presentation.value = this.read(this.options.value, false);
        presentation.active = this.read(this.options.active, null);
        presentation.enabled = this.read(this.options.enabled, true);
        presentation.loading = this.read(this.options.loading, false);
        presentation.mode = this.options.mode === "momentary"
            ? "momentary" : "toggle";
        presentation.label = this.options.label === undefined
            ? "" : String(this.options.label);
        let scope = this.options.scope;
        presentation.scopeActive = Boolean(scope && typeof scope.isGroup === "function" &&
            scope.isGroup());
        presentation.scopeColor = presentation.scopeActive ? scope.color : null;
        this.publish(presentation);
    }
    
    setValue(value, transactionId)
    {
        presentationBindingWrite(this.options.value, value, transactionId);
    }
    
    resetValue()
    {
        let source = presentationBindingSource(this.options.value);
        if (source && typeof source.reset === "function") {
            source.reset();
        }
    }
    
    destroy()
    {
        if (this.destroyed) return;
        for (let index = 0; index < this.unsubscribers.length; index += 1) {
            this.unsubscribers[index]();
        }
        this.unsubscribers = [];
        super.destroy();
    }
}


module.exports = {
    ButtonPresenter: ButtonPresenter
};
