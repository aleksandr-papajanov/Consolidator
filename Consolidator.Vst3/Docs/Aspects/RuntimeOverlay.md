# Runtime Leveler overlay

## Input Leveler

The accepted `gain_staging.input_leveler` flag controls editability of the Level dial.
While enabled, its outer ring displays `gain_staging.input_level` from the existing
`dsp_telemetry` frame and is read-only. The separate fast correction
`gain_staging.input_level_fast` draws a smaller relative arc centered at zero over
the existing -6/+6 dB display range. It does not move the outer knob.

ChannelStrip/GainStaging/Clients/InputTelemetryClient decodes and validates the
wire telemetry; GainStaging bindings own its presentation. Telemetry is accepted
only for the current Gain Staging target and a newer revision,
after its snapshot is ready and audio is prepared. Both values are validated
before updating the presentation. Target/audio-configuration transitions clear
the runtime display; disabling Leveler hides the inner arc and restores the
accepted static level. A Leveler enable notification cancels a manual level
gesture. The displayed runtime pair is separate from `ParameterEntry` and is
never submitted through `write`, history or persistence.

Disabling Leveler sends an ordinary `gain_staging.input_leveler = false` write.
Managed prepares each grouped instance's own slow gain and flag in one transaction.
The frontend submits no dependent gain writes and owns no history policy.

