# Runtime Metrics Diagnostics

Managed collects runtime counters continuously without involving the UI or the
realtime audio callback. `RuntimeMetricsMonitor` samples those counters every
five seconds after the first baseline sample.

The monitor reports only actionable conditions:

- dropped audio samples observed during a sampling interval;
- Managed native-input average above 5 ms with at least four calls in the interval;
- Managed control-operation average above 5 ms with at least four operations in the interval.

Each condition has an independent 30-second reporting cooldown. Normal
snapshots, allocation totals, registry activity, presentation discards and
zero-valued counters are not posted. The monitor does not post recovery or
heartbeat messages.

The `metrics` selector remains an explicit native diagnostic request and is not
sent by Max UI intent handling. Its snapshot is intended for manual
investigation, not continuous monitoring.
