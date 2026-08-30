class BankManagerPresentation
{
    constructor()
    {
        this.enabled = true;
        this.selectedPanel = "equalizer";
        this.rows = [];
        this.groupAction = null;
        this.ungroupAction = null;
        this.clearAction = { enabled: false };
        this.scopeAction = {
            enabled: false,
            active: false,
            color: null
        };
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
