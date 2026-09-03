const { UiColors } = require("../../../Shared/Theme/UiColors.js");
const { BankManagerControlOptions } = require("./BankManagerControlOptions.js");
const { BankManagerGridRenderer } = require("./BankManagerGridRenderer.js");
const { BankManagerActionRenderer } = require("./BankManagerActionRenderer.js");
const {
    fillRectangle,
    groupLabel,
    isGroupedBank
} = require("./BankManagerDrawing.js");

class BankManagerRenderer
{
    constructor()
    {
        this.gridRenderer = new BankManagerGridRenderer();
        this.actionRenderer = new BankManagerActionRenderer();
    }

    paint(graphics, presentation, layout, feedback)
    {
        let rows = presentation.rows || [];
        let highlightedGroups = this.highlightedGroups(presentation);
        fillRectangle(graphics, BankManagerControlOptions.background,
            0, 0, graphics.size[0], graphics.size[1]);
        graphics.translate(BankManagerControlOptions.outerPadding,
            BankManagerControlOptions.outerPadding);
        fillRectangle(graphics, BankManagerControlOptions.background,
            0, 0, layout.width, layout.height);
        graphics.select_font_face("Arial");
        graphics.set_font_size(BankManagerControlOptions.fontSize);
        this.actionRenderer.paint(graphics, presentation, layout, feedback);
        this.gridRenderer.paint(graphics, rows, layout);
        rows.forEach((row, rowIndex) => {
            this.paintRow(graphics, presentation, row, rowIndex,
                layout, feedback, highlightedGroups);
        });
        graphics.translate(-BankManagerControlOptions.outerPadding,
            -BankManagerControlOptions.outerPadding);
    }

    highlightedGroups(presentation)
    {
        let highlighted = {};
        if (!Boolean((presentation.scopeAction || {}).active)) return highlighted;
        (presentation.rows || []).forEach((row) => {
            (row.banks || []).forEach((bank) => {
                if ((bank.active || bank.selected) && isGroupedBank(bank)) {
                    highlighted[String(bank.groupId)] = true;
                }
            });
        });
        return highlighted;
    }

    paintRow(graphics, presentation, row, rowIndex, layout, feedback, highlighted)
    {
        let options = BankManagerControlOptions;
        let y = rowIndex * options.rowHeight - layout.scrollPosition;
        if (y + options.rowHeight < 0 || y + options.rowHeight > layout.height) return;
        graphics.set_source_rgba.apply(graphics,
            row.local ? options.focused : options.remote);
        graphics.set_font_size(options.fontSize);
        let label = String(row.label || "");
        let labelWidth = layout.labelWidth() - 2;
        while (label.length > 0 && graphics.text_measure(label)[0] > labelWidth) {
            label = label.substring(0, label.length - 1);
        }
        graphics.move_to(2, y + 12);
        graphics.show_text(label);
        this.paintProcessorMarkers(
            graphics, presentation, row, y, layout, feedback);
        (row.banks || []).forEach((bank, bankIndex) => {
            let isHighlighted = Boolean((presentation.scopeAction || {}).active)
                ? highlighted[String(bank.groupId)] === true
                : Boolean(bank.active);
            this.paintBank(graphics, bank,
                layout.bankGridX() + bankIndex * (options.bankSize + options.bankGap),
                y, isHighlighted, row.instanceId, feedback);
        });
        this.paintInstanceButtons(graphics, presentation, row,
            layout.instanceButtonsX(), y, feedback);
    }

    paintBank(graphics, bank, x, y, highlighted, instanceId, feedback)
    {
        if (!bank.visible) return;
        let options = BankManagerControlOptions;
        let active = bank.active || bank.selected || highlighted;
        let fallbackColor = bank.system ? options.text : options.focused;
        let color = bank.selected
            ? options.deviceColors.equalizer : bank.color || fallbackColor;
        let alpha = bank.opacity === undefined ? 1 : bank.opacity;
        let displayColor = [color[0], color[1], color[2],
            (color[3] === undefined ? 1 : color[3]) * alpha *
                (bank.active ? 1 : 0.8)];
        let selectionColor = bank.effectActive ? displayColor : options.separator;
        let fallbackTextColor = active
            ? bank.effectActive ? options.background : options.disabled
            : displayColor;
        let textColor = bank.textColor || fallbackTextColor;
        let bankHeight = active ? options.rowHeight : options.bankSize;
        if (active) fillRectangle(graphics, selectionColor,
            x, y, options.bankSize, bankHeight);
        if (bank.effectActive) {
            this.paintActivityMarker(graphics,
                bank.bypassed ? options.disabled : options.deviceColors.equalizer,
                x, y);
        }
        if (!isGroupedBank(bank)) return;
        graphics.set_source_rgba.apply(graphics, textColor);
        graphics.select_font_face("Arial");
        graphics.set_font_size(9);
        let label = groupLabel(bank.groupId);
        let textSize = graphics.text_measure(label);
        let fontExtents = graphics.font_extents();
        graphics.move_to(x + (options.bankSize - textSize[0]) / 2,
            y + (bankHeight - fontExtents[2]) / 2 + fontExtents[0]);
        graphics.show_text(label);
    }

    paintProcessorMarkers(graphics, presentation, row, y, layout, feedback)
    {
        let options = BankManagerControlOptions;
        options.processorMarkerIds.forEach((processorId, index) => {
            let processor = (row.processors || []).find((candidate) => {
                return candidate.processorId === processorId;
            }) || { markerActive: false };
            let x = layout.markerGridX() + index * options.bankSize;
            let color = options.deviceColors[processorId];
            let selectedInstance = (row.banks || []).some((bank) => bank.active);
            let selected = selectedInstance &&
                String(presentation.selectedPanel || "").toLowerCase() === processorId;
            if (selected) fillRectangle(graphics,
                processor.effectActive ? color : options.separator,
                x, y, options.bankSize, options.bankSize);
            if (processor.markerActive) {
                this.paintActivityMarker(graphics,
                    processor.bypassed ? options.disabled : color, x, y);
            }
        });
    }

    paintActivityMarker(graphics, color, x, y)
    {
        fillRectangle(graphics, BankManagerControlOptions.background,
            x + 1, y + 1, 5, 5);
        fillRectangle(graphics, color, x + 2, y + 2, 3, 3);
    }

    paintInstanceButtons(graphics, presentation, row, x, y, feedback)
    {
        let options = BankManagerControlOptions;
        let buttons = [
            { label: "S", active: Boolean(row.solo), color: options.solo },
            { label: "M", active: Boolean(row.mute), color: options.mute },
            { label: "R", active: feedback.isActionFlashed(
                "instance:" + row.instanceId), color: UiColors.devices.reset },
            { label: "B", active: Boolean(row.bypass), color: UiColors.devices.reset }
        ];
        buttons.forEach((button, index) => {
            let buttonX = x + index * options.bankSize;
            if (button.active) fillRectangle(graphics, button.color,
                buttonX, y, options.bankSize, options.rowHeight);
            let scope = presentation.scopeAction || {};
            let textColor = scope.active && scope.color
                ? scope.color
                : button.active ? options.background : UiColors.base.activeText;
            graphics.set_source_rgba.apply(graphics, textColor);
            graphics.select_font_face("Arial");
            graphics.set_font_size(9);
            let textSize = graphics.text_measure(button.label);
            graphics.move_to(buttonX + (options.bankSize - textSize[0]) / 2, y + 12);
            graphics.show_text(button.label);
        });
    }
}

module.exports = {
    BankManagerRenderer: BankManagerRenderer,
    groupLabel: groupLabel,
    isGroupedBank: isGroupedBank
};
