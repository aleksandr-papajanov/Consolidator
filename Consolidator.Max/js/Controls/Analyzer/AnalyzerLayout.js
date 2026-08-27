class AnalyzerLayout
{
    constructor(width, height)
    {
        this.left = width * 0.08;
        this.top = height * 0.08;
        this.width = width * 0.88;
        this.height = height * 0.84;
    }
}

module.exports = {
    AnalyzerLayout: AnalyzerLayout
};
