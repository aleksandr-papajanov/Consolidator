class MultiValueTogglePresentation
{
    constructor()
    {
        this.enabled = true;
        this.active = true;
        this.value = 0;
        this.values = [];
        this.scopeActive = false;
        this.scopeColor = null;
    }
}

module.exports = { MultiValueTogglePresentation: MultiValueTogglePresentation };
