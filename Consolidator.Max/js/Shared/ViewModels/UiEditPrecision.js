const { PresentationObservable } = require("../Presenters/PresentationObservable.js");
const { ButtonPresentation } = require("../Controls/Button/ButtonPresentation.js");

class UiEditPrecision extends PresentationObservable
{
    constructor()
    {
        super();
        this.mode = "rough";
        this.rebuild();
    }

    isRough()
    {
        return this.mode === "rough";
    }

    toggle()
    {
        this.setValue(!this.isRough());
    }

    setValue(rough)
    {
        let nextMode = Boolean(rough) ? "rough" : "precise";
        if (this.mode === nextMode) return;
        this.mode = nextMode;
        this.rebuild();
    }

    rebuild()
    {
        let presentation = new ButtonPresentation();
        presentation.value = this.isRough();
        presentation.active = this.isRough();
        presentation.enabled = true;
        presentation.label = this.isRough() ? "ROUGH" : "PRECISE";
        this.publish(presentation);
    }

    setValueFromControl(value)
    {
        this.setValue(value);
    }
}

module.exports = {
    UiEditPrecision: UiEditPrecision,
    uiEditPrecision: new UiEditPrecision()
};
