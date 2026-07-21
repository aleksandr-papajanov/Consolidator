# ProcessorControls wiring

`ProcessorControls.maxpat` owns the controller and the Host bus connection. Add
ordinary Max controls inside this patcher and keep every parameter control in
the normalized `0..1` range.

Connect control outputs to the controller's first inlet using these messages:

```text
preeq bypass <0|1>
preeq reset
posteq bypass <0|1>
posteq reset

filter <filterId> <parameter> <normalized>
filter <filterId> bypass <0|1>
filter <filterId> reset

input_gain <normalized>

compressor attack <normalized>
compressor release <normalized>
compressor threshold <normalized>
compressor bypass <0|1>
compressor reset

saturator saturation <normalized>
saturator bypass <0|1>
saturator reset

output_gain <normalized>
```

For example, filter 3 gain uses `[prepend filter 3 gain]`, compressor attack
uses `[prepend compressor attack]`, and a reset button triggers the literal
message `filter 3 reset`.

Frequency controls always use the semantic name `frequency`. The controller
maps it to the filter definition's absolute `freq` or `pivot` parameter.

The section bypass/reset controls operate on the selected bank. Reset restores
only that section's filters and bypass to typed defaults in one Host transaction.

The controller's second outlet sends confirmed normalized state directly to
`thispatcher`:

```text
script sendbox preeq.bypass set <0|1>
script sendbox posteq.bypass set <0|1>
script sendbox filter.<filterId>.<parameter|bypass> set <0..1>
script sendbox input_gain.gain set <0..1>
script sendbox compressor.<attack|release|threshold|bypass> set <0..1>
script sendbox saturator.<saturation|bypass> set <0..1>
script sendbox output_gain.gain set <0..1>
```

Connect the controller's second outlet to `thispatcher`. The `set` message
updates the corresponding control without feeding its value back into the
command inlet.

The controller's fourth and fifth outlets publish confirmed processor state
for the compact compressor and saturator JSUI response views respectively:

```text
compressor_state <bypass> <attackMs> <releaseMs> <thresholdDb>
saturator_state <bypass> <saturation>
```

Both views show static transfer characteristics from the Host-confirmed DSP
snapshot. They do not synthesize gain-reduction or waveform activity.

Suggested stable varnames are `filter.<id>.<parameter>`,
`filter.<id>.bypass`, `filter.<id>.reset`, `preeq.bypass`, `preeq.reset`,
`posteq.bypass`, `posteq.reset`,
`input_gain.gain`, `compressor.attack`,
`compressor.release`, `compressor.threshold`, `compressor.bypass`,
`saturator.saturation`, `saturator.bypass`, and `output_gain.gain`.

PreEq contains tilt 2 and bells 4-5. PostEq contains shelves 3 and 6 plus
bells 7-9. DSP applies PreEq before compressor/saturator and PostEq after them.
