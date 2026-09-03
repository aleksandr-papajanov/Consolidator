const { StateValueViewModel } = require("../../../Shared/ViewModels/StateValueViewModel.js");

class PolishViewModel
{
    constructor(state)
    {
        this.thick = new StateValueViewModel(state, "polish.thick");
        this.air = new StateValueViewModel(state, "polish.air");
    }

    getStateValues()
    {
        return [this.thick, this.air];
    }

    destroy()
    {
        this.thick.destroy();
        this.air.destroy();
    }
}

module.exports = { PolishViewModel: PolishViewModel };
