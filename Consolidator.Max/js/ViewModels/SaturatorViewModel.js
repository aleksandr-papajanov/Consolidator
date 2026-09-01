const { StateValueViewModel } = require("./StateValueViewModel.js");
const { DetectorFilterViewModel } = require("./DetectorFilterViewModel.js");
const { DetectorFilterDefinitions } = require("./FilterCatalog.js");

class SaturatorViewModel
{
    constructor(state)
    {
        this.drive = new StateValueViewModel(state, "saturator.drive");
        this.curve = new StateValueViewModel(state, "saturator.curve");
        this.split = new StateValueViewModel(state, "saturator.split");
        this.output = new StateValueViewModel(state, "saturator.output");
        this.detectorFilters = [1, 2].map((filterId) => {
            return new DetectorFilterViewModel(state, "saturator", filterId,
                DetectorFilterDefinitions[filterId - 1]);
        });
    }
    
    getStateValues()
    {
        return [
            this.drive,
            this.curve,
            this.split,
            this.output
        ].concat(this.detectorFilters.reduce((values, filter) => {
            return values.concat(filter.getStateValues());
        }, []));
    }
    
    destroy()
    {
        this.drive.destroy();
        this.curve.destroy();
        this.split.destroy();
        this.output.destroy();
        this.detectorFilters.forEach((filter) => { filter.destroy(); });
    }
}


module.exports = {
    SaturatorViewModel: SaturatorViewModel
};
