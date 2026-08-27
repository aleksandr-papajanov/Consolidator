class DialPresentation
{
    constructor()
    {
        this.enabled = true;
        this.loading = false;
        this.active = true;
        this.activeIndex = 0;
        this.displayIndex = 0;
        this.rings = [];
    }
}

module.exports = {
    DialPresentation: DialPresentation
};
