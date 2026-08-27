const { DialPresenter } = require("../Presenters/Dial/DialPresenter.js");
const { ButtonPresenter } = require("../Presenters/Button/ButtonPresenter.js");
const { bindPresentation } = require("../Presenters/Core/PresentationBinding.js");

class FeaturePresenterSet
{
    constructor()
    {
        this.presenters = {};
    }
    
    addDial(name, source, display)
    {
        let presenter = new DialPresenter({
            rings: [{
                value: source,
                display: display || {}
            }],
            enabled: this.sourceProperty(source, "enabled", true),
            loading: this.sourceProperty(source, "loading", false)
        });
        this.presenters[name] = { type: "dial", presenter: presenter };
        return presenter;
    }
    
    addButton(name, source, label)
    {
        let presenter = new ButtonPresenter({
            value: source,
            label: label || "",
            enabled: this.sourceProperty(source, "enabled", true),
            loading: this.sourceProperty(source, "loading", false)
        });
        this.presenters[name] = { type: "button", presenter: presenter };
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

