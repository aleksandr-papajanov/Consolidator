# Native test architecture

The test suite mirrors the production boundaries instead of grouping unrelated
checks in large scenario executables.

```text
Tests/
├─ Component/
│  ├─ Analysis/      latest-value transport, FFT and response calculations
│  ├─ Core/          state, queues, routing and runtime resolution
│  ├─ Dsp/           processors, filters, chain construction and routing
│  └─ Max/           atom protocol codecs, framing and wire correlation
├─ Commands/         one suite per state-protocol command family
├─ Integration/      coordinator, protocol and audio-block boundaries
└─ Support/          test runner, path builders and focused fixtures
```

## Test levels

- `component` tests instantiate one production component or one tightly coupled
  component cluster. They do not wait for the coordinator worker.
- `command` tests call `CommandRouter` synchronously and verify each command
  handler's response and side effects.
- `integration` tests use public `ConsolidatorInstance` APIs, the coordinator
  worker, runtime mailboxes and audio processing together.

CTest labels match these levels. After building with the repository build
script, run all tests or a level from the configured build directory:

```text
ctest -C RelWithDebInfo --output-on-failure
ctest -C RelWithDebInfo -L component --output-on-failure
ctest -C RelWithDebInfo -L command --output-on-failure
ctest -C RelWithDebInfo -L integration --output-on-failure
```

## Conventions

- One `TEST_CASE` verifies one behavior and has a sentence-style name.
- Shared setup belongs in `Support`; production behavior must not be duplicated
  in helpers.
- Component tests use synchronous APIs. Waiting/polling is confined to
  `ProtocolDriver`, which is used only by integration tests.
- Analysis integration tests cover global view switching, view epochs,
  main/reference/difference spectra, individual EQ curves, the current-bank
  aggregate, the all-banks aggregate, and spectrum accumulator resets when a
  slot is reactivated.
- Exact paths, response statuses and persistent state are asserted separately
  from derived runtime effects.
- New production components and command variants require a matching focused
  suite and, when they cross boundaries, an integration scenario.
