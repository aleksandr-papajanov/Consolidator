const { StateValueViewModel } = require("../../../Shared/ViewModels/StateValueViewModel.js");
const { DetectorFilterViewModel } = require("../../../Shared/ViewModels/DetectorFilterViewModel.js");
const { DetectorFilterDefinitions } = require("../../../Shared/ViewModels/FilterCatalog.js");

class InputViewModel
{
    constructor(state)
    {
        this.targetState = state;
        this.level = new StateValueViewModel(state, "input_gain.level");
        this.target = new StateValueViewModel(state, "input_gain.target");
        this.width = new StateValueViewModel(state, "input_gain.width");
        this.leveler = new StateValueViewModel(state, "input_gain.leveler");
        this.detectorFilters = [1, 2].map((filterId) => new DetectorFilterViewModel(
            state,
            "input_gain",
            filterId,
            DetectorFilterDefinitions[filterId - 1]
        ));
    }

    destroy()
    {
        this.level.destroy();
        this.target.destroy();
        this.width.destroy();
        this.leveler.destroy();
        this.detectorFilters.forEach((filter) => filter.destroy());
    }
}

module.exports = {
    InputViewModel: InputViewModel
};
