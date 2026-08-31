# Max JavaScript tests

```text
tests/
├─ support/                 production loader and state fixtures
├─ suites/client/
│  ├─ UiProtocolTests.js             JS → wire frame → JS response
│  ├─ UiBindingTests.js              UI intent and presentation use cases
│  ├─ MultiValueToggleTests.js       multi-value control intent and presentation
│  ├─ RegistryAndBankManagerTests.js registry and topology workflows
│  ├─ LiveInstanceHostTests.js       Live identity/activity lifecycle
│  ├─ RuntimePathTests.js             complete Max package contract
│  └─ ClientSuite.js                 exhaustive suite registry
├─ ClientTests.js                         stable test entrypoint
└─ max9/
   ├─ Max9HostTests.maxpat                 Max 9 v8/v8ui host harness
   ├─ Max9HostTestRunner.js                 v8 patcher/dispatch checks
   └─ V8UiTestProbe.js                      v8ui mgraphics/Task checks
```

The root files are stable entry points used by developer commands and CTest.
Shared environment setup and fixtures live in `support`; test implementations
live in `suites`. Every suite is registered explicitly by `ClientSuite.js`; an
unregistered test file is not considered part of the test system.

Run from the repository root:

```text
node Consolidator.Max/tests/ClientTests.js
```

Max 9 host mode: open `Consolidator.Max/tests/max9/Max9HostTests.maxpat` in
Max 9 / Live 12 and inspect its outlet results. Node and Max 9 modes are
separate by design: only the Max host can exercise `patcher`, `mgraphics`,
`Task` and native object message dispatch.

