const { UiColors } = require("../../../Shared/Theme/UiColors.js");
const { BankManagerControlOptions } = require("./BankManagerControlOptions.js");
const { fillRectangle } = require("./BankManagerDrawing.js");

class BankManagerActionRenderer
{
    paint(graphics, presentation, layout, feedback)
    {
        let actions = [
            { key: "group", label: "Group", action: presentation.groupAction || {}, momentary: true },
            { key: "ungroup", label: "Ungroup", action: presentation.ungroupAction || {}, momentary: true },
            { key: "clear", label: "Clear", action: presentation.clearAction || {}, momentary: true },
            {
                key: "scope",
                label: (presentation.scopeAction || {}).active ? "Group" : "Local",
                action: presentation.scopeAction || {},
                momentary: false
            }
        ];
        fillRectangle(graphics, BankManagerControlOptions.background,
            layout.actionsColumnX(), 0,
            BankManagerControlOptions.actionColumnWidth, layout.height);
        actions.forEach((entry, index) => {
            this.paintButton(graphics, entry,
                Math.round(index * (BankManagerControlOptions.actionButtonHeight +
                    BankManagerControlOptions.actionGap)), layout, feedback);
        });

        let history = presentation.history || {};
        let historyY = layout.actionGroupHeight() +
            BankManagerControlOptions.historyGroupGap;
        [
            { key: "historyRedo", label: "Redo", action: { enabled: Boolean(history.canRedo) }, momentary: true },
            { key: "historyUndo", label: "Undo", action: { enabled: Boolean(history.canUndo) }, momentary: true }
        ].forEach((entry, index) => {
            this.paintButton(graphics, entry,
                historyY + index * (BankManagerControlOptions.actionButtonHeight +
                    BankManagerControlOptions.actionGap), layout, feedback);
        });
    }

    paintButton(graphics, entry, y, layout, feedback)
    {
        let options = BankManagerControlOptions;
        let x = layout.actionsColumnX();
        let bottom = Math.round(y + options.actionButtonHeight);
        let height = bottom - y;
        let flashed = entry.momentary && feedback.isActionFlashed(entry.key);
        let fillColor = flashed
            ? options.focused
            : entry.action.active && entry.action.color
                ? entry.action.color : options.background;
        let textColor = entry.action.enabled
            ? UiColors.base.activeText : UiColors.base.disabledText;

        fillRectangle(graphics, fillColor, x, y, options.actionColumnWidth, height);
        fillRectangle(graphics, options.separator, x, y, options.actionColumnWidth, 1);
        fillRectangle(graphics, options.separator,
            x, bottom - 1, options.actionColumnWidth, 1);
        fillRectangle(graphics, options.separator, x, y, 1, height);
        fillRectangle(graphics, options.separator,
            x + options.actionColumnWidth - 1, y, 1, height);
        graphics.set_source_rgba.apply(graphics,
            flashed || entry.action.active ? options.background : textColor);
        graphics.select_font_face("Arial");
        graphics.set_font_size(9);
        let textSize = graphics.text_measure(entry.label);
        graphics.move_to(x + (options.actionColumnWidth - textSize[0]) / 2, y + 12);
        graphics.show_text(entry.label);
    }
}

module.exports = {
    BankManagerActionRenderer: BankManagerActionRenderer
};
