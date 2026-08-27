class SliderPresentation
{
    constructor()
    {
        this.enabled = true;
        this.active = true;
        this.value = 0;
        this.minimum = 0;
        this.maximum = 1;
        this.orientation = "horizontal";
        this.display = { value: "" };
        this.color = null;
    }
}

module.exports = {
    SliderPresentation: SliderPresentation
};
