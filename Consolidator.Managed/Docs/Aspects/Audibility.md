# Audibility projection

`AudibilityObserver` stores only the observed local instance mute/solo values and
target `DspRuntimeState` references. On mute, solo or instance lifecycle
changes it recalculates effective audibility: mute always disables its
instance, and any active solo disables every non-solo instance. Solo changes
never write temporary mute values back to persistent state, so removing solo
restores each instance's original mute state. Bank groups select the
instances mutated by an explicit control command; they are not part of the
audibility calculation. The former general
`StateValueProjection`, `AudibilityResolver` and `ProcessingStateResolver`
layers no longer exist.

