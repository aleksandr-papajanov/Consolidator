const { StateValueViewModel } = require("./StateValueViewModel.js");
const { DetectorFilterViewModel } = require("./DetectorFilterViewModel.js");

class SaturatorViewModel
{
    constructor(state)
    {
        this.drive = new StateValueViewModel(state, "saturator.drive");
        this.mix = new StateValueViewModel(state, "saturator.mix");
        this.gain = new StateValueViewModel(state, "saturator.gain");
        this.detectorAmount = new StateValueViewModel(
            state,
            "saturator.detector_amount"
        );
        this.bypass = new StateValueViewModel(state, "saturator.bypass");
        this.solo = new StateValueViewModel(state, "saturator.solo");
        this.detectorListen = new StateValueViewModel(
            state,
            "saturator.detector.listen"
        );
        this.detectorFilters = [1, 2].map((filterId) => {
            return new DetectorFilterViewModel(state, "saturator", filterId);
        });
    }
    
    getStateValues()
    {
        return [
            this.drive,
            this.mix,
            this.gain,
            this.detectorAmount,
            this.bypass,
            this.solo,
            this.detectorListen
        ].concat(this.detectorFilters.reduce((values, filter) => {
            return values.concat(filter.getStateValues());
        }, []));
    }
    
    destroy()
    {
        this.drive.destroy();
        this.mix.destroy();
        this.gain.destroy();
        this.detectorAmount.destroy();
        this.bypass.destroy();
        this.solo.destroy();
        this.detectorListen.destroy();
        this.detectorFilters.forEach((filter) => { filter.destroy(); });
    }
}


module.exports = {
    SaturatorViewModel: SaturatorViewModel
};
