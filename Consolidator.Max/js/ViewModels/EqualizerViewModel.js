const { StateValueViewModel } = require("./StateValueViewModel.js");
const { FilterViewModel } = require("./FilterViewModel.js");
const { EqualizerFilterDefinitions } = require("./FilterCatalog.js");

class EqualizerViewModel
{
    constructor(state)
    {
        this.state = state;
        this.filters = [1, 2, 3, 4, 5, 6, 7].map((filterId) => {
            return new FilterViewModel(state, filterId,
                EqualizerFilterDefinitions[filterId - 1]);
        });
    }
    
    destroy()
    {
        this.filters.forEach((filter) => { filter.destroy(); });
        this.filters = [];
    }
}


module.exports = {
    EqualizerViewModel: EqualizerViewModel
};
