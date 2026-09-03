const { BankManagerControlOptions } = require("./BankManagerControlOptions.js");

class BankManagerFeedback
{
    constructor(redraw, taskFactory)
    {
        this.redraw = redraw;
        this.taskFactory = taskFactory;
        this.actionFlash = {};
        this.actionFlashTasks = {};
        this.bypassOverrides = {};
        this.destroyed = false;
    }

    flashAction(actionId)
    {
        if (this.destroyed)
        {
            return;
        }

        const key = String(actionId);
        const previous = this.actionFlashTasks[key];
        if (previous)
        {
            previous.cancel();
        }

        this.actionFlash[key] = true;
        this.redraw();
        const task = this.taskFactory(() => {
            if (this.destroyed)
            {
                return;
            }

            delete this.actionFlash[key];
            delete this.actionFlashTasks[key];
            this.redraw();
        });
        this.actionFlashTasks[key] = task;
        task.schedule(BankManagerControlOptions.actionFlashDurationMs);
    }

    isActionFlashed(actionId)
    {
        return Boolean(this.actionFlash[String(actionId)]);
    }

    bypassKey(instanceId, itemId)
    {
        return String(instanceId) + ":" + String(itemId);
    }

    bypassValue(instanceId, itemId, value)
    {
        const key = this.bypassKey(instanceId, itemId);
        return this.bypassOverrides[key] === undefined
            ? Boolean(value)
            : this.bypassOverrides[key];
    }

    setBypassOverride(instanceId, itemId, value)
    {
        this.bypassOverrides[this.bypassKey(instanceId, itemId)] = Boolean(value);
    }

    confirmBypassOverride(instanceId, itemId, value)
    {
        const key = this.bypassKey(instanceId, itemId);
        if (this.bypassOverrides[key] === Boolean(value))
        {
            delete this.bypassOverrides[key];
        }
    }

    destroy()
    {
        if (this.destroyed)
        {
            return;
        }

        this.destroyed = true;
        Object.keys(this.actionFlashTasks).forEach((key) => {
            this.actionFlashTasks[key].cancel();
        });
        this.actionFlash = {};
        this.actionFlashTasks = {};
        this.bypassOverrides = {};
    }
}

module.exports = {
    BankManagerFeedback: BankManagerFeedback
};
