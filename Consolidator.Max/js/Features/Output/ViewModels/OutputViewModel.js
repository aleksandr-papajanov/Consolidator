const { StateValueViewModel } = require("../../../Shared/ViewModels/StateValueViewModel.js");

class OutputViewModel
{
    constructor(state)
    {
        this.level = new StateValueViewModel(state, "output_gain.level");
        this.target = new StateValueViewModel(state, "output_gain.target");
        this.limiter = new StateValueViewModel(state, "output_gain.limiter");
    }

    destroy()
    {
        this.level.destroy();
        this.target.destroy();
        this.limiter.destroy();
    }
}

module.exports = {
    OutputViewModel: OutputViewModel
};
