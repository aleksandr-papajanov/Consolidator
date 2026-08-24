# Max JavaScript tests

```text
tests/
├─ support/                 production loader and state fixtures
├─ suites/client/
│  ├─ UiProtocolTests.js             JS → wire frame → JS response
│  ├─ UiBindingTests.js              UI intent and presentation use cases
│  ├─ RegistryAndBankManagerTests.js registry and topology workflows
│  ├─ LiveTrackIdentityHostTests.js  Live identity lifecycle
│  ├─ RuntimePathTests.js             complete Max package contract
│  └─ ClientSuite.js                 exhaustive suite registry
└─ ClientTests.js                         stable test entrypoint
```

The root files are stable entry points used by developer commands and CTest.
Shared environment setup and fixtures live in `support`; test implementations
live in `suites`. Every suite is registered explicitly by `ClientSuite.js`; an
unregistered test file is not considered part of the test system.

Run from the repository root:

```text
node Consolidator.Max/tests/ClientTests.js
```

