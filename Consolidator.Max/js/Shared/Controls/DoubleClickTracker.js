const DOUBLE_CLICK_INTERVAL_MS = 350;

class DoubleClickTracker
{
    constructor()
    {
        this.lastKey = null;
        this.lastTime = 0;
    }

    isDoubleClick(key)
    {
        let now = Date.now();
        let isDouble = this.lastKey === key &&
            now - this.lastTime <= DOUBLE_CLICK_INTERVAL_MS;
        this.lastKey = key;
        this.lastTime = now;
        return isDouble;
    }
}

module.exports = {
    DoubleClickTracker: DoubleClickTracker
};
