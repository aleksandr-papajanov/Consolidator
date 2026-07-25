# Interface

## Purpose

`Interface` is the JSUI-based visual layer for Consolidator controls. It owns
presentation, interaction, and control-specific layout. It does not own DSP,
device state, persistence, or message-bus routing.

## Visual Decisions

- Shared colors and line widths live in `JS/InterfaceVisualConfig.js`.
- Components use the shared visual configuration by default.
- A component may override a visual value only when its geometry or purpose
  requires a different treatment. The override must stay local to that
  component.
- Every interface element must be responsive to the dimensions of its own
  container. Drawing, hit testing, labels, markers, and control ranges derive
  from the current bounds rather than a fixed design-time size. Absolute values
  are allowed for visual stroke widths and other deliberate pixel-level
  styling.
- New controls use the same neutral dark background, cyan active value, and
  light marker unless a deliberate local override is documented here.
- Line widths are absolute pixel values, not ratios of the control size.
- Controls remain responsive to their own JSUI bounds.
- Dial values are normalized to `0..1` at the UI boundary.
- Static control configuration is passed through `jsui` attributes and saved in
  the patcher. Messages remain available for runtime value changes and output.
- Reuse existing interface components and models whenever their behavior fits.
  Do not rewrite the same interaction or state logic in a new control. If a
  behavior can be shared by multiple controls, extract it into a focused
  reusable class immediately.

## Base Visual Configuration

The current shared defaults are:

- background: `[0.08, 0.08, 0.08, 1.0]`
- track: `[0.20, 0.20, 0.20, 1.0]`
- active value: `[0.10, 0.78, 0.92, 1.0]`
- signed indicator: `[0.98, 0.72, 0.18, 1.0]`
- alert: `[0.92, 0.18, 0.14, 1.0]`
- text: `[0.92, 0.92, 0.92, 1.0]`
- border: `[0.34, 0.34, 0.34, 1.0]`
- control font size ratio: `0.30`
- dial value font size ratio: `0.16`
- control line width: `2.0`
- indicator line width: `1.5`
- border line width: `1.0`

The source of truth is `JS/InterfaceVisualConfig.js`; this list is a readable
record of the current design decisions and must be updated with it.

## Dial

`JS/DialControl.js` is the first control. Its value is changed by vertical
drag, clamped to `0..1`, and emitted through its only outlet. The dial sweep is
centered at the top with a 120-degree unused section at the bottom. The
`emptySectionAngle` setting controls that gap.

Dial also supports multiple concentric values:

- `valueCount`: creates up to three concentric dials;
- `primaryValue`, `secondaryValue`, `tertiaryValue`: set the normalized
  values without output;
- `primaryIndicator`, `secondaryIndicator`, `tertiaryIndicator`: set the
  initial signed indicator values for the corresponding rings;
- `indicator <ring-index> <-1..1>`: draws a secondary signed arc next to the
  selected ring, starting at the center and extending left or right;
- `clearIndicator <ring-index>`: hides that ring's secondary arc;
- `visualization <ring-index> <none|signed|color|relative> <value>`: selects a generic
  per-ring visualization. `signed` draws the secondary arc with `-1..1`;
  `color` blends the active ring from the shared value color to alert red with
  `0..1`; `relative` draws an independent arc from the ring's current position
  by `-1..1` of the full sweep; `none` disables all visualization on the ring.
- `enabled`: enables or disables pointer interaction;
- `set <index> <value>`: sets one one-based dial value without output;
- a single dial outputs a float;
- multiple dials output `[index, value]` on every drag.

The first value is edited by default. Hold Ctrl while dragging to edit the
second value, or Alt while dragging to edit the third value. Ring selection by
mouse position is not used.

Double-click resets the selected value to its default. Ctrl selects the second
value for reset and Alt selects the third value.

The active ring is resolved on every drag event, so Ctrl or Alt can be pressed
or released during the same gesture without starting a new click.

The dial normally displays only the first value. While editing the second or
third value, it displays the active value and returns to the first value one
second after the last change.

`JS/SliderControl.js` is a horizontal normalized slider. Its track, active
segment, and marker derive their positions from the current JSUI bounds. Its
value is configured with the `value` attribute.
The `enabled` attribute and the `enable` / `disable` messages control pointer
interaction.

`JS/ButtonControl.js` supports two modes:

- `mode toggle`: each click switches and retains the value;
- `mode momentary`: press outputs `1`, release outputs `0`.

The mode and label are configured with the `mode` and `label` attributes. The
normalized state can be initialized with the `value` attribute.
The `enabled` attribute and the `enable` / `disable` messages control pointer
interaction.

`JS/ButtonGroupControl.js` composes buttons in one responsive control. Its
configuration attributes are:

- `count`: number of buttons;
- `layout`: `horizontal` or `vertical` button arrangement;
- `selectionMode`: `single`, `multiple`, or `custom` selection behavior;
- `labels`: button labels;
- `selection`: initial one-based selection without output;
- `allowEmptySelection`: in `single` mode, allows all buttons to be off;
- `buttonModes`: per-button `toggle` or `momentary` modes in `custom` mode;
- `enabled`: enables or disables pointer interaction.
- `loadingIndex`: one-based button index shown as loading, or `0` to clear it.

Disabling a group releases an active momentary button before blocking pointer
interaction. A loading button remains visually highlighted while the group is
disabled.

The old runtime selectors remain useful for live control from a patcher inlet:

- `count <number>`: number of buttons;
- `layout horizontal|vertical`: button arrangement;
- `selectionMode single|multiple|custom`: selection behavior;
- `allowEmptySelection 0|1`: configure an empty selection in `single` mode;
- `buttonModes <toggle|momentary...>`: configure `custom` button modes;
- `labels <label...>`: button labels;
- `set <index>`: set selection without output.

Selection indices are one-based. In `single` mode the outlet emits one index,
or `0` when nothing is selected. In `multiple` mode the outlet emits a list of
selected indices.

In `custom` mode the outlet emits `[index, value]` for both toggle and
momentary buttons.
All controls also accept `enabled 0|1`, `enable`, and `disable` messages.

All controls also accept `outputValue`, which emits the current state without
changing it. A multi-value Dial and a custom ButtonGroup emit one indexed pair
per value.
