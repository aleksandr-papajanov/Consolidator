const { StateValueViewModel } = require("./StateValueViewModel.js");
const { DetectorFilterViewModel } = require("./DetectorFilterViewModel.js");
const { DetectorFilterDefinitions } = require("./FilterCatalog.js");

class GainViewModel
{
    constructor(state, path)
    {
        this.targetState = state;
        this.level = new StateValueViewModel(state, path + ".level");
        this.target = new StateValueViewModel(state, path + ".target");
        if (path.indexOf("input") >= 0) {
            this.width = new StateValueViewModel(state, path + ".width");
        }
        let toggleName = path.indexOf("input") >= 0 ? "leveler" : "limiter";
        this[toggleName] = new StateValueViewModel(state, path + "." + toggleName);
        this.detectorListen = path.indexOf("input") >= 0
            ? new StateValueViewModel(state, "input_gain.detector.listen") : null;
        this.detectorFilters = path.indexOf("input") >= 0
            ? [1, 2].map((filterId) => new DetectorFilterViewModel(
                state, "input_gain", filterId,
                DetectorFilterDefinitions[filterId - 1])) : [];
    }
    
    getStateValues()
    {
        return [this.level, this.target, this.width, this.leveler || this.limiter,
            this.detectorListen]
            .concat(this.detectorFilters.reduce((values, filter) => {
                return values.concat(filter.getStateValues());
            }, []))
            .filter((value) => value);
    }
    
    destroy()
    {
        this.level.destroy();
        this.target.destroy();
        if (this.width) this.width.destroy();
        (this.leveler || this.limiter).destroy();
        if (this.detectorListen) this.detectorListen.destroy();
        this.detectorFilters.forEach((filter) => filter.destroy());
    }
}


module.exports = {
    GainViewModel: GainViewModel
};
