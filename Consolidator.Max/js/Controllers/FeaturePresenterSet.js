include("Project:/js/Presenters/Dial/DialPresenter.js");
include("Project:/js/Presenters/Button/ButtonPresenter.js");
include("Project:/js/Presenters/Core/PresentationBinding.js");

function FeaturePresenterSet() {
    this.presenters = {};
}

FeaturePresenterSet.prototype.addDial = function (name, source, display) {
    var presenter = new DialPresenter({
        rings: [{
            value: source,
            display: display || {}
        }],
        enabled: this.sourceProperty(source, "enabled", true),
        loading: this.sourceProperty(source, "loading", false)
    });
    this.presenters[name] = { type: "dial", presenter: presenter };
    return presenter;
};

FeaturePresenterSet.prototype.addButton = function (name, source, label) {
    var presenter = new ButtonPresenter({
        value: source,
        label: label || "",
        enabled: this.sourceProperty(source, "enabled", true),
        loading: this.sourceProperty(source, "loading", false)
    });
    this.presenters[name] = { type: "button", presenter: presenter };
    return presenter;
};

FeaturePresenterSet.prototype.sourceProperty = function (
    source, propertyName, fallback
) {
    return bindPresentation(source, {
        read: function () {
            return source[propertyName] === undefined
                ? fallback : source[propertyName];
        }
    });
};

FeaturePresenterSet.prototype.forEach = function (callback) {
    Object.keys(this.presenters).forEach(function (name) {
        var entry = this.presenters[name];
        callback(name, entry.presenter, entry.type);
    }, this);
};

FeaturePresenterSet.prototype.destroy = function () {
    Object.keys(this.presenters).forEach(function (name) {
        this.presenters[name].presenter.destroy();
    }, this);
    this.presenters = {};
};
