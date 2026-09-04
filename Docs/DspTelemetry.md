# DSP Telemetry

## Scope

DSP telemetry is a transient, latest-only presentation channel from the native
audio runtime to the active Max UI. It reports slowly changing runtime values
that controls may display while an algorithm owns their motion. It is not a
second authoritative state tree and it is not used for lossless commands,
history, persistence, or DSP parameter publication.

```text
Native audio-owned processor state and final output
    -> seqlock-style DspTelemetryExchange
    -> Managed DspTelemetryService worker
    -> dsp_telemetry / track_levels latest-only frames
    -> independent native latest telemetry slots
    -> analysis outlet
    -> TargetStateClient / TrackLevelClient presentation caches
```

The fixed shared exchange contains the Leveler `InputLevel` and
`InputLevelFast` values, final `OutputLevel`, and a presence mask. Additional
telemetry values extend that bounded snapshot and one of the versioned telemetry
frames; they do not create per-parameter callbacks or queues.

## Realtime boundary

At the end of each processed block Native writes the telemetry snapshot with an
odd/even sequence counter. The audio thread performs only bounded scalar and
atomic operations. It does not allocate, lock, call Managed, schedule Max work,
or format protocol atoms.

The Managed worker polls at 33 ms and validates the sequence before and after
copying the snapshot. Focused Leveler telemetry follows the same focused-bank
source selection as the spectrum analyzer. While a UI viewer is active, final
output levels from every registered instance are batched into one `track_levels`
frame. Inactive UI sessions create no Managed protocol frames and no Max
scheduler work. Native retains only the newest pending frame for each telemetry
kind, independently of the newest FFT frame.

Registration associates each instance with its native-owned exchange pointer.
Unregistration removes that pointer under the reader lock before the native
external can release the exchange, so the worker cannot retain or read a
borrowed pointer beyond the unregister barrier.

The wire frame is:

```text
dsp_telemetry version sourceInstanceId revision valueCount path value ...
```

The Bank Manager meter frame is a complete latest snapshot:

```text
track_levels version revision instanceCount instanceId outputLevelDb ...
```

Each native instance measures linked mean-square energy after the complete DSP
graph and converts its 300 ms exponential RMS average to dBFS. The value is
published even when the Leveler is disabled. It is runtime telemetry only and
never enters state, history, persistence, grouping, or DSP parameter snapshots.
The Bank Manager maps `-60..0 dBFS` to `0..1` and raises the normalized value
to `1.8`, using the result as the alpha of a six-pixel lime circle beside the
track name. Level updates use a dedicated row patch and do not rebuild registry
state.

`TargetStateClient` accepts only the current target source and monotonically
newer revisions. Telemetry replaces the displayed value while preserving the
state entry's range and status metadata.

For the Leveler, one frame contains both values:

- `input_gain.level` is the slow long-term gain and drives the outer dial ring.
  The fast controller never changes this displayed knob value.
- `input_gain.level_fast` is the fast correction relative to the slow operating
  point. It is limited to `-6..6 dB` and rendered as a smaller lime inner arc
  extending left or right from the zero center. The arc is hidden when Leveler
  is disabled.

The fast telemetry field is presentation-only even though the reported
correction participates in native audio processing. It does not change the
editability or authoritative Managed state of `input_gain.level`.

## Leveler control ownership

While Leveler is enabled, `input_gain.level` is read-only in the UI and its dial
displays the native Leveler's slow long-term gain from telemetry. The separately
rendered fast correction is applied only inside the native Leveler and does not
move the knob. The Managed state value remains the last committed value while
automation is running.

Leveler enable and disable use the current UI edit scope. Enabling uses the
ordinary grouped state write because only the boolean `leveler` flag is copied;
the existing live `state_changed` path updates the button immediately. Every
Leveler continues to measure and control its own audio independently.

When Leveler is turned off, the dedicated `disable_input_leveler` command resolves
the selected bank group without ordinary peer-value copying. Managed reads the
latest slow level from each target instance's native telemetry exchange and
commits those distinct values with the flags in one history transaction. An
instance without a valid telemetry value retains its existing Input Gain.
Managed then publishes each retained value as ordinary static Input Gain, while
Native removes that instance's fast overlay through its bounded input-gain
ramp. The independently committed values participate in normal history and
persistence.
