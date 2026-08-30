class ButtonPresentation
{
    constructor()
    {
        this.enabled = true;
        this.loading = false;
        this.active = false;
        this.value = false;
        this.mode = "toggle";
        this.label = "";
        this.scopeActive = false;
        this.scopeColor = null;
    }
}

module.exports = {
    ButtonPresentation: ButtonPresentation
};
