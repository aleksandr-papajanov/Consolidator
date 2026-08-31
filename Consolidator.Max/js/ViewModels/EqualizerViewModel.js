const { StateValueViewModel } = require("./StateValueViewModel.js");
const { FilterViewModel } = require("./FilterViewModel.js");
const { EqualizerFilterDefinitions } = require("./FilterCatalog.js");

class EqualizerViewModel
{
    constructor(state)
    {
        this.state = state;
        this.bypass = new StateValueViewModel(state, "equalizer.bypass");
        this.bankBypass = new StateValueViewModel(state, "equalizer.bank.bypass");
        this.bankSolo = new StateValueViewModel(state, "equalizer.bank.solo");
        this.filters = [1, 2, 3, 4, 5, 6, 7].map((filterId) => {
            return new FilterViewModel(state, filterId,
                EqualizerFilterDefinitions[filterId - 1]);
        });
    }
    
    destroy()
    {
        this.bypass.destroy();
        this.bankBypass.destroy();
        this.bankSolo.destroy();
        this.filters.forEach((filter) => { filter.destroy(); });
        this.filters = [];
    }
}


module.exports = {
    EqualizerViewModel: EqualizerViewModel
};
