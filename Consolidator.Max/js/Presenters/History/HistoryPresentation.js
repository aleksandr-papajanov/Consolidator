class HistoryPresentation
{
    constructor()
    {
        this.entries = [];
        this.cursor = 0;
        this.undo = { enabled: false, label: "Undo" };
        this.redo = { enabled: false, label: "Redo" };
    }
}

module.exports = {
    HistoryPresentation: HistoryPresentation
};
