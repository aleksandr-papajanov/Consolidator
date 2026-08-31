const { StateValueViewModel } = require("./StateValueViewModel.js");
const { DetectorFilterViewModel } = require("./DetectorFilterViewModel.js");
const { DetectorFilterDefinitions } = require("./FilterCatalog.js");

class CompressorViewModel
{
    constructor(state)
    {
        this.attack = new StateValueViewModel(state, "compressor.attack");
        this.sustain = new StateValueViewModel(state, "compressor.sustain");
        this.compression = new StateValueViewModel(state, "compressor.compression");
        this.character = new StateValueViewModel(state, "compressor.character");
        this.parallel = new StateValueViewModel(state, "compressor.parallel");
        this.output = new StateValueViewModel(state, "compressor.output");
        this.detectorListen = new StateValueViewModel(
            state,
            "compressor.detector.listen"
        );
        this.detectorFilters = [1, 2].map((filterId) => {
            return new DetectorFilterViewModel(state, "compressor", filterId,
                DetectorFilterDefinitions[filterId - 1]);
        });
    }
    
    getStateValues()
    {
        return [
            this.attack,
            this.sustain,
            this.compression,
            this.character,
            this.parallel,
            this.output,
            this.detectorListen
        ].concat(this.detectorFilters.reduce((values, filter) => {
            return values.concat(filter.getStateValues());
        }, []));
    }
    
    destroy()
    {
        this.attack.destroy();
        this.sustain.destroy();
        this.compression.destroy();
        this.character.destroy();
        this.parallel.destroy();
        this.output.destroy();
        this.detectorListen.destroy();
        this.detectorFilters.forEach((filter) => { filter.destroy(); });
    }
}


module.exports = {
    CompressorViewModel: CompressorViewModel
};
