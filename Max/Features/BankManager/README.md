# Bank Manager

`consolidator.bankmanager.js` owns feature composition and local BankManager
behavior. Runtime-only cross-instance topology belongs to the native
process-local `LinkCoordinator`; BankManager receives its compact directory and
does not own remote state, resolve Live identity, or implement JSUI input
dispatch directly.

## Components

- `JS/BankManagerModels.js`: mutable local UI summaries.
- `JS/BankManagerSelection.js`: active/focused bank state and deterministic
  peer-row ordering.
- `JS/BankManagerLayout.js`: bank-list geometry, link-panel layout, hit-testing,
  editable group identifiers, and link colors.
- `JS/BankManagerOperations.js`: discrete link assignment, filter resets, and
  Join/Commit/Reset/Bypass transactions, including their global replication.
- `JS/BankManagerLiveIdentity.js`: Live API device identity, track order, and
  track-name observation.
- `JS/BankManagerMessageRouter.js`: Max selector/list routing to local UI
  operations, Coordinator directory data, or discrete global operations.
- `JS/BankManagerUiController.js`: JSUI paint, click, and scroll handling.
- `Max/Shared/Runtime/LinkRevisionTracker.js`: reusable monotonic revision
  tracking for global link update and operation streams.

The public atom protocol remains defined in `AGENTS.md`. New BankManager logic
belongs in the smallest component that owns its responsibility; do not add new
Max callbacks or Live API calls to the entrypoint unless they are wiring only.
