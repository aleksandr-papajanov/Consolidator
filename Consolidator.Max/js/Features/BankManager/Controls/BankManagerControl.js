autowatch = 1;
inlets = 1;
outlets = 2;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { BankManagerControlOptions } = require("./BankManagerControlOptions.js");
const { BankManagerControlCore } = require("./BankManagerControlCore.js");

class BankManagerControl extends BankManagerControlCore
{
    constructor()
    {
        super(
            mgraphics,
            (name, payload) => {
                if (payload === undefined)
                {
                    outlet(0, name);
                }
                else if (payload instanceof Array)
                {
                    outlet(0, [name].concat(payload));
                }
                else
                {
                    outlet(0, [name, payload]);
                }
            },
            Task
        );
    }
}

function presentation_begin(enabled)
{
    bankManagerControl.beginPresentation(enabled);
}

function bank_bypass(value)
{
    bankManagerControl.setBankBypass(value);
}

function row(...args)
{
    bankManagerControl.addRow(...args);
}

function processor(...args)
{
    bankManagerControl.addProcessor(...args);
}

function bank(...args)
{
    bankManagerControl.addBank(...args);
}

function group_action(enabled, active)
{
    bankManagerControl.setGroupAction(enabled, active);
}

function ungroup_action(enabled, active)
{
    bankManagerControl.setUngroupAction(enabled, active);
}

function clear_action(enabled)
{
    bankManagerControl.setClearAction(enabled);
}

function scope_action(enabled, active, hasColor, red, green, blue, alpha)
{
    bankManagerControl.setScopeAction(enabled, active, hasColor, red, green, blue, alpha);
}

function history(cursor, entryCount, canUndo, canRedo)
{
    bankManagerControl.setHistory(cursor, entryCount, canUndo, canRedo);
}

function selected_panel(panel)
{
    bankManagerControl.setSelectedPanel(panel);
    outlet(1, panel);
}

function presentation_end()
{
    bankManagerControl.endPresentation();
}

function presentation_patch_begin(enabled)
{
    bankManagerControl.beginPresentationPatch(enabled);
}

function bank_bypass_patch(value)
{
    bankManagerControl.patchBankBypass(value);
}

function row_patch(...args)
{
    bankManagerControl.patchRow(...args);
}

function processor_patch(...args)
{
    bankManagerControl.patchProcessor(...args);
}

function row_remove(index)
{
    bankManagerControl.removeRow(index);
}

function bank_patch(...args)
{
    bankManagerControl.patchBank(...args);
}

function group_action_patch(enabled, active)
{
    bankManagerControl.patchGroupAction(enabled, active);
}

function ungroup_action_patch(enabled, active)
{
    bankManagerControl.patchUngroupAction(enabled, active);
}

function clear_action_patch(enabled)
{
    bankManagerControl.patchClearAction(enabled);
}

function scope_action_patch(enabled, active, hasColor, red, green, blue, alpha)
{
    bankManagerControl.patchScopeAction(enabled, active, hasColor, red, green, blue, alpha);
}

function history_patch(cursor, entryCount, canUndo, canRedo)
{
    bankManagerControl.patchHistory(cursor, entryCount, canUndo, canRedo);
}

function presentation_patch_end()
{
    bankManagerControl.endPresentationPatch();
}

function paint()
{
    bankManagerControl.paint();
}

function onresize()
{
    mgraphics.redraw();
}

function onclick(x, y, button, modifier1, shift, caps, option, modifier2)
{
    bankManagerControl.beginPointer(x, y, shift);
    bankManagerControl.selectAt(
        x,
        y,
        shift,
        Number(modifier1) !== 0 || Number(modifier2) !== 0
    );
    bankManagerControl.interaction.markClickHandled();
}

function ondrag(x, y, button)
{
    if (button === 0)
    {
        bankManagerControl.endPointer(x, y);
    }
    else
    {
        bankManagerControl.movePointer(x, y);
    }
}

function onidleout()
{
    bankManagerControl.cancelPointer();
}

function onwheel(x, y, delta)
{
    if (!bankManagerControl.presentation.enabled)
    {
        return;
    }

    const wheelDelta = Number(delta);
    if (isFinite(wheelDelta))
    {
        bankManagerControl.scrollBy(-wheelDelta * BankManagerControlOptions.rowHeight);
    }
}

function notifydeleted()
{
    bankManagerControl.destroy();
}

const bankManagerControl = new BankManagerControl();
