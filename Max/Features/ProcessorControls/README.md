# ProcessorControls wiring

`ProcessorControls.maxpat` owns the controller and the Host bus connection. Add
ordinary Max controls inside this patcher and keep every parameter control in
the normalized `0..1` range.

Connect control outputs to the controller's first inlet using these messages:

```text
eq bypass <0|1>
eq reset

filter <filterId> <parameter> <normalized>
filter <filterId> bypass <0|1>
filter <filterId> reset

input_gain <normalized>

compressor attack-release <1|2> <normalized>
compressor input-output <1|2> <normalized>
compressor mix <normalized>
compressor mode <1..3>
compressor detector <1|2> <gain|frequency|q|bypass> <normalized>
compressor bypass <0|1>
compressor reset

saturator input-output <1|2> <normalized>
saturator mix <normalized>
saturator mode <1..3>
saturator detector <1|2> <gain|frequency|q|bypass> <normalized>
saturator bypass <0|1>
saturator reset

output_gain <normalized>
```

`Gain.maxpat` is the reusable input/output gain control. Instantiate it with
`input` or `output`. Its two DialControl rings are gain and target RMS. The
component listens to `---processor.telemetry`, displays the post-stage RMS as a
signed indicator next to the gain ring relative to the target ring, and sends only `gain.set_parameter`
to Host. The target range is `-60..0 dB`; target and meter state are local UI
state and do not enter the DSP snapshot.

`Gains.maxpat` composes the two standard instances side by side: input first,
output second.

The input gain target is published over the scoped
`---input.gain.target` UI transport. Compressor and saturator controllers
combine it with their measured output RMS from `---processor.telemetry` and
draw the same signed indicator next to ring 2 of their `inputOutput` dials.
Compressor gain reduction is drawn on the output ring in the shared reduction
color. Its arc starts at the current output position and extends backwards by
the measured reduction.
Saturator nonlinear ratio drives the input ring's generic `color`
visualization from the shared active color toward alert red.
`Shared/JS/SaturationVisualization.js` applies a sensitivity curve before
exponential smoothing. `sensitivity` is normalized: `0` is linear and `1`
makes the visualization approach red within the lowest part of the range.
`smoothing` controls visual inertia.
These UI transports never enter Host or the runtime atom bus.

`consolidator.processorcontrols.detectorcurve.js` renders both detector bell
filters in a compact view. It receives confirmed absolute detector values from
the DSP snapshot through `thispatcher`, draws the individual responses and
markers, and overlays their summed response. Its only command is:

```text
detector <1|2> <bypass> <gainDb> <frequencyHz> <q>
```

For example, filter 3 gain uses `[prepend filter 3 gain]`, compressor attack
uses `[prepend compressor attack]`, and a reset button triggers the literal
message `filter 3 reset`.

Frequency controls always use the semantic name `frequency`. The controller
maps it to the filter definition's absolute `freq` or `pivot` parameter.

The EQ bypass/reset controls operate on the selected bank. Reset restores all
filters and the chain bypass to typed defaults in one Host transaction.

The controller's second outlet sends confirmed normalized state directly to
`thispatcher`:

```text
script sendbox eq.bypass set <0|1>
script sendbox filter.<filterId>.<parameter|bypass> set <0..1>
script sendbox input_gain.gain set <0..1>
script sendbox compressor.attackRelease set <1|2> <0..1>
script sendbox compressor.inputOutput set <1|2> <0..1>
script sendbox compressor.<mix|bypass> set <0..1>
script sendbox compressor.mode set <1..3>
script sendbox compressor.detector.<1|2>.<gain|frequency|q|bypass> set <0..1>
script sendbox saturator.inputOutput set <1|2> <0..1>
script sendbox saturator.<mix|bypass> set <0..1>
script sendbox saturator.mode set <1..3>
script sendbox saturator.detector.<1|2>.<gain|frequency|q|bypass> set <0..1>
script sendbox output_gain.gain set <0..1>
```

Connect the controller's second outlet to `thispatcher`. The `set` message
updates the corresponding control without feeding its value back into the
command inlet.

Compressor and saturator activity is measured from processed audio by
DspProcessor and displayed in SpectrumView. ProcessorControls does not render
transfer functions or publish visual state.

Suggested stable varnames are `filter.<id>.<parameter>`,
`filter.<id>.bypass`, `filter.<id>.reset`, `eq.bypass`, `eq.reset`,
`input_gain.gain`, `compressor.attackRelease`, `compressor.inputOutput`,
`compressor.mix`, `compressor.mode`, `compressor.control`,
`compressor.detector.1`, `compressor.detector.2`, `saturator.inputOutput`,
`saturator.mix`, `saturator.mode`,
`saturator.control`, `saturator.detector.1`, `saturator.detector.2`, and
`output_gain.gain`.

All EQ filters belong to one ordered chain. DSP applies compressor and saturator
first, then all EQ banks in ascending bank/filter order.

## Eq Dial Controller

`consolidator.eq.controller.js` is the controller for `Eq.maxpat`. Each Dial
represents one filter and uses rings in this order: `1` gain, `2` frequency
(`freq`/`pivot`), and optional `3` q. Shelves and tilt expose only two rings;
their shelf Q is fixed in DSP settings. Filter `9` is the selected bank's
constant gain and exposes only the gain ring in the `-15..15 dB` range. It is
part of the EQ definition catalog and is fitted by `match eq` together with
the frequency-dependent filters. Connect a Dial through `prepend dial <filterId>` and a
ButtonGroup through `prepend control <filterId>` to the controller's local
inlet. Button `1` is bypass and button `2` is reset.
The global `Bypass/Reset` group uses `prepend eqglobal`; button `1` applies
chain bypass, and button `2` resets the complete EQ chain.

The controller listens to Host `snapshot` messages through
`r ---message.bus.out`. Its first outlet publishes commands through
`s ---message.bus.in`. Its second outlet is connected to the local
`thispatcher` using these varnames:

```text
eq.filter.<id>.dial
eq.filter.<id>.control
```

The Dial receives `primaryValue`, `secondaryValue`, and `tertiaryValue`
attributes. The control group receives the `selection` attribute.
