const { StateValueViewModel } = require("../../../Shared/ViewModels/StateValueViewModel.js");
const { DetectorFilterViewModel } = require("../../../Shared/ViewModels/DetectorFilterViewModel.js");
const { DetectorFilterDefinitions } = require("../../../Shared/ViewModels/FilterCatalog.js");
const { AutomatedLevelViewModel } = require("./AutomatedLevelViewModel.js");
const { LevelerViewModel } = require("./LevelerViewModel.js");

class InputViewModel
{
    constructor(state)
    {
        this.targetState = state;
        this.levelState = new StateValueViewModel(state, "input_gain.level");
        this.fastLevelState = new StateValueViewModel(state, "input_gain.level_fast");
        this.target = new StateValueViewModel(state, "input_gain.target");
        this.width = new StateValueViewModel(state, "input_gain.width");
        this.levelerState = new StateValueViewModel(state, "input_gain.leveler");
        this.level = new AutomatedLevelViewModel(this.levelState, this.levelerState);
        this.leveler = new LevelerViewModel(state, this.levelerState);
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
        this.levelState.destroy();
        this.fastLevelState.destroy();
        this.target.destroy();
        this.width.destroy();
        this.leveler.destroy();
        this.levelerState.destroy();
        this.detectorFilters.forEach((filter) =>
        {
            filter.destroy();
        });
    }
}

module.exports = {
    InputViewModel: InputViewModel
};
