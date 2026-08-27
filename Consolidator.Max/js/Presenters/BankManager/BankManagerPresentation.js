class BankManagerPresentation
{
    constructor()
    {
        this.enabled = true;
        this.rows = [];
        this.linkEditing = false;
        this.linkGroups = [];
        this.editAction = null;
        this.clearAction = null;
        this.delta = null;
    }
}

module.exports = {
    BankManagerPresentation: BankManagerPresentation
};
