class BankManagerPresentation
{
    constructor()
    {
        this.enabled = true;
        this.selectedPanel = "input";
        this.rows = [];
        this.groupAction = null;
        this.ungroupAction = null;
        this.clearAction = null;
        this.history = {
            cursor: 0,
            entryCount: 0,
            canUndo: false,
            canRedo: false
        };
        this.delta = null;
    }
}

module.exports = {
    BankManagerPresentation: BankManagerPresentation
};
