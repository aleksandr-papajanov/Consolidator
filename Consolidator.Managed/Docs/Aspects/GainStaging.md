# Gain Staging state and DSP projection

Gain Staging replaces the separate Input/Output application domains. Its processor
ID and state-path prefix are both `gain_staging`. There are no old ID/path aliases
or persistence adapters. Physical input/output processing stays at the existing
positions before/after the other processors; UI consolidation does not reorder audio.

## Authoritative state

`DspState.GainStaging` owns one `GainStagingState`. ManagedStateBuilder constructs
every leaf through the existing StateValueFactory with instance ownership and
connected edit scope. The ordinary peer, state-change, activity and deferred DSP
publication observers remain in use.

| Relative leaf | Type | Default | Physical range |
| --- | --- | --- | --- |
| input_level | float, dB | 0 | -36..36 |
| input_target | float, dB | -18 | -36..0 |
| input_leveler | bool | false | boolean |
| output_level | float, dB | 0 | -36..36 |
| output_target | float, dB | -1 | -36..0 |
| output_leveler | bool | false | boolean |
| limit | float, dB | -1 | -36..0 |
| width | float | 1 | 0..1 |
| bypass | bool | false | boolean |

The former input detector remains `gain_staging.detector.filter.*`, using the
same filter definitions and first detector slots. It is still editable through
the shared graphical analyzer. New numeric controls and both Level flags are
history-backed; Bypass retains the standard non-history processor-control policy.
Scalar gestures retain begin/coalesced writes/final acknowledgment and copy/delta
scope selection. Managed determines discrete history from the changed state values.
Both Level directions use ordinary state writes. On `input_leveler = false`,
Managed `GainStagingEdits` prepares each enabled target's own latest slow level
alongside the flags in the same transaction. Without valid telemetry its manual
level stays unchanged. An explicit input-level value in the same batch takes
precedence over the derived freeze. Resetting the individual Level flag has the
same freeze behavior; resetting the device subtree restores its defaults.
Manual input-level writes while a targeted Leveler is active require disabling
it in the same batch; this constraint is enforced in Managed as well as the UI.
Output Level has no fabricated freeze/telemetry operation while its DSP is absent.

Presentation geometry and bindings: [VST3 ChannelStrip](../../../Consolidator.Vst3/Docs/Aspects/ChannelStrip.md).

## Runtime snapshot and current sound support

StateProjectionObserver projects all new values into DspRuntimeState and its
published DspSnapshot through the existing change tracker/triple-buffer exchange.
Both physical bypass bits derive from one Gain Staging Bypass value. The output
limiter enable bit is derived from that bypass: Limit is always active when the
combined device is enabled, since there is no separate limiter toggle in this UI.
The existing limiter reads OutputLimit instead of OutputTarget. Its audio
algorithm is unchanged. OutputTarget is reserved for output automatic leveling.

Input Level/Target/Leveler, Width and manual Output Level retain their existing
DSP consumers. OutputLeveler and OutputTarget reach the Native snapshot, but
output automatic leveling is intentionally unimplemented at this UX stage.
There is no placeholder audio algorithm or alternative runtime path.

Managed/Native DspSnapshot append uint OutputLeveler and float OutputLimit at
offsets 1476 and 1480. Size is 1484 bytes. DspStateExchange size is 4480, with
publishedIndex/consumerIndex/telemetry at 4452/4456/4460. Existing physical-stage
field offsets remain unchanged. Native static assertions and existing ABI fixture
expectations describe the same layout; this is not a sound-algorithm test.

## Persistence

Strict persistence schema 6 stores one gainStaging object with every leaf and its
detector filters. Restore validates ranges/shape before applying one baseline
history transaction and publishing affected snapshots. This also fixes the old
Input detector omission. Schema 5 and old Input/Output objects are not decoded.
