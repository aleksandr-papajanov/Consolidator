# Compression state and DSP projection

Compression is an independent processor with the state prefix `compression`.
Its state values are `type`, `reduction`, `output`, `attack`, `release`,
`ratio`, `parallel` and `bypass`. Per-type profiles remain hidden state under
`compression.profile` and are switched atomically by Managed.
Defaults, physical ranges and history policy belong to the independent
`StateValueDefinitions.Compression` catalog.
Bypass defaults to false, matching other devices. Processor reset restores the
device to its enabled state.

Types 0–3 select SSL, 76, 3A and 2A. Local/copy edits update the selected
profile together with active values. Group delta edits preserve each instance's
offset in both places. A parameter reset updates the selected profile; a type
reset restores each target's default-type profile. A processor or DSP reset
resets all profiles and active values. These changes share one history transaction.
Profiles are internal storage and are not exposed as protocol mutation paths.
Their changes still mark persistence dirty, but do not emit presentation notifications.
StateChangeRouter excludes internal profile paths from presentation routing;
the protocol publisher retains its ordinary persistence and encoding responsibilities.

The processor has its own target observation context, bypass/activity
projection and persistence object. Its values continue to use the existing
fixed Native snapshot transport while the audio consumer remains deferred.
