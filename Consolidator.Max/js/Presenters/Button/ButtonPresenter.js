include("../Core/PresentationObservable.js");
include("../Core/PresentationBinding.js");
include("ButtonPresentation.js");

function ButtonPresenter(options) {
    PresentationObservable.call(this);
    this.options = options || {};
    this.unsubscribers = [];
    this.subscribeSources();
    this.rebuild();
}

ButtonPresenter.prototype = Object.create(PresentationObservable.prototype);
ButtonPresenter.prototype.constructor = ButtonPresenter;

ButtonPresenter.prototype.read = function (source, fallback) {
    return presentationBindingValue(source, fallback);
};

ButtonPresenter.prototype.subscribeSources = function () {
    var self = this;
    var sources = [this.options.value, this.options.enabled, this.options.active];
    for (var index = 0; index < sources.length; index += 1) {
        subscribePresentationBinding(sources[index], function () {
            self.rebuild();
        }, this.unsubscribers);
    }
};

ButtonPresenter.prototype.rebuild = function () {
    var presentation = new ButtonPresentation();
    presentation.value = this.read(this.options.value, false);
    presentation.active = this.read(this.options.active, null);
    presentation.enabled = this.read(this.options.enabled, true);
    presentation.mode = this.options.mode === "momentary"
        ? "momentary" : "toggle";
    presentation.label = this.options.label === undefined
        ? "" : String(this.options.label);
    this.publish(presentation);
};

ButtonPresenter.prototype.setValue = function (value) {
    presentationBindingWrite(this.options.value, value);
};

ButtonPresenter.prototype.resetValue = function () {
    var source = presentationBindingSource(this.options.value);
    if (source && typeof source.reset === "function") {
        source.reset();
    }
};

ButtonPresenter.prototype.destroy = function () {
    if (this.destroyed) return;
    for (var index = 0; index < this.unsubscribers.length; index += 1) {
        this.unsubscribers[index]();
    }
    this.unsubscribers = [];
    PresentationObservable.prototype.destroy.call(this);
};
