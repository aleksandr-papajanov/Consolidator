const { StateValueViewModel } = require("./StateValueViewModel.js");

class GainViewModel
{
    constructor(state, path)
    {
        this.gain = new StateValueViewModel(state, path + ".gain");
    }
    
    getStateValues()
    {
        return [this.gain];
    }
    
    destroy()
    {
        this.gain.destroy();
    }
}


module.exports = {
    GainViewModel: GainViewModel
};
