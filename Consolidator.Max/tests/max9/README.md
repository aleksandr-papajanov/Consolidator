# Max 9 host tests

`Max9HostTests.maxpat` is the host-side test mode. Open it in Max 9 / Live 12
and inspect the outlet messages:

The harness uses paths relative to its own patcher, so it can be opened
directly without an existing Max project context.

- `max9_host_test passed` confirms `v8` loading, `patcher.getnamed` and message
  dispatch to the named `v8ui` object;
- `max9_ui_test scheduled` confirms the `v8ui` probe accepted a `Task` and
  requested a repaint;
- `max9_ui_test passed` confirms the scheduled callback ran and repainted;
- the probe's `paint` and `onresize` callbacks exercise the graphics boundary.

The same statuses are written to `Max9HostTestResult.txt` next to the harness,
so an automated run does not depend on scraping the Max Console.

The Node mode remains `node Consolidator.Max/tests/ClientTests.js` and covers
deterministic CommonJS behavior: panel routing, snapshot batching, lazy
activation, target switching, transaction lifecycle, one rebuild per
presenter, analyzer task teardown, dial presentation comparison, malformed
messages and destruction/recreation.

The Max 9 mode is intentionally separate because Node cannot provide Max's
`patcher`, `mgraphics`, `Task` or object message dispatch semantics. It must be
run manually in the supported host; no `js`/`jsui` fallback is part of the
harness.
