const { DialPresenter } = require("../Presenters/Dial/DialPresenter.js");
const { ButtonPresenter } = require("../Presenters/Button/ButtonPresenter.js");
const { MultiValueTogglePresenter } = require("../Presenters/MultiValueToggle/MultiValueTogglePresenter.js");
const { bindPresentation } = require("../Presenters/Core/PresentationBinding.js");

class FeaturePresenterSet
{
    constructor(scope)
    {
        this.presenters = {};
        this.scope = scope;
    }
    
    addDial(name, source)
    {
        let presenter = new DialPresenter({
            rings: [{
                value: source
            }],
            scope: this.scope,
            enabled: this.sourceProperty(source, "enabled", true),
            loading: this.sourceProperty(source, "loading", false)
        });
        this.presenters[name] = { type: "dial", presenter: presenter };
        return presenter;
    }

    addToggle(name, source, label)
    {
        let presenter = new ButtonPresenter({
            value: source,
            label: label || "",
            scope: this.scope,
            enabled: this.sourceProperty(source, "enabled", true),
            loading: this.sourceProperty(source, "loading", false)
        });
        this.presenters[name] = { type: "toggle", presenter: presenter };
        return presenter;
    }

    addMultiValueToggle(name, source, values)
    {
        let presenter = new MultiValueTogglePresenter({
            value: source,
            values: values,
            scope: this.scope,
            enabled: this.sourceProperty(source, "enabled", true),
            loading: this.sourceProperty(source, "loading", false)
        });
        this.presenters[name] = { type: "multiValueToggle", presenter: presenter };
        return presenter;
    }
    
    sourceProperty(
        source, propertyName, fallback
    )
    {
        return bindPresentation(source, {
            read: () => {
                return source[propertyName] === undefined
                    ? fallback : source[propertyName];
            }
        });
    }
    
    forEach(callback)
    {
        Object.keys(this.presenters).forEach((name) => {
            let entry = this.presenters[name];
            callback(name, entry.presenter, entry.type);
        }, this);
    }
    
    destroy()
    {
        Object.keys(this.presenters).forEach((name) => {
            this.presenters[name].presenter.destroy();
        }, this);
        this.presenters = {};
    }
}

module.exports = {
    FeaturePresenterSet: FeaturePresenterSet
};
