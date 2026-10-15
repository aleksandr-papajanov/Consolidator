# Shaper state and DSP projection

Shaper is an independent processor with the state prefix `shaper`. Its state
values are `attack`, `sustain`, `output` and `bypass`. It is independently
observable, resettable, bypassable and persisted under the `shaper` property
of the DSP persistence object.

Defaults, physical ranges and history policy belong to
`StateValueDefinitions.Shaper`; state composition only attaches observers.
Bypass defaults to false, matching other devices. Processor reset restores the
device to its enabled state.

The Managed projection publishes the existing shaper transport fields in the
fixed Native snapshot. SnapshotBinding maps them to an independent ShaperDevice
after Saturation and before Compression. It owns bypass/activity transitions and
calls the current TransientShaper algorithm with its own HP20 detector.
