const { StateValueViewModel } = require("../../../Shared/ViewModels/StateValueViewModel.js");
const { FilterViewModel } = require("../../../Shared/ViewModels/FilterViewModel.js");
const { EqualizerFilterDefinitions } = require("../../../Shared/ViewModels/FilterCatalog.js");

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
