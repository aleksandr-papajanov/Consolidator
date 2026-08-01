# Bank Manager

`consolidator.bankmanager.js` owns feature composition and BankManager domain
coordination. It does not decode raw snapshots, resolve Live identity, or
implement JSUI input dispatch directly.

## Components

- `JS/BankManagerModels.js`: mutable local and peer summaries plus processor
  link-group delta math.
- `JS/BankManagerMath.js`: normalized/absolute parameter conversion.
- `JS/BankManagerDefinitions.js`: static UI definitions projected into filter
  metadata, processor ranges, and reset defaults.
- `JS/BankManagerLinkGraph.js`: the derived cross-instance link graph and
  processor groups built from compact peer topology.
- `JS/BankManagerSelection.js`: active/focused bank state and deterministic
  peer-row ordering.
- `JS/BankManagerLayout.js`: bank-list geometry, link-panel layout, hit-testing,
  editable group identifiers, and link colors.
- `JS/BankManagerOperations.js`: discrete link assignment, filter resets, and
  Join/Commit/Reset/Bypass transactions, including their global replication.
- `JS/BankManagerLinkTransport.js`: global bank announcements, linked-state
  frames, relative parameter deltas, discrete linked filter operations, and
  the local preview lane for linked controls.
- `JS/BankManagerLinkPresentation.js`: linked-control colors, fixed control
  limits, the selected-bank control session, detector previews, and
  SpectrumView ghost-filter previews.
- `JS/BankManagerSnapshotReader.js`: validation and decoding of EQ and
  processor Host snapshots into feature models.
- `JS/BankManagerSnapshotCoordinator.js`: applies decoded snapshots, refreshes
  the derived link graph, publishes topology summaries, and refreshes scoped
  control presentation after canonical state changes.
- `JS/BankManagerLiveIdentity.js`: Live API device identity, track order, and
  track-name observation.
- `JS/BankManagerMessageRouter.js`: Max selector/list routing to BankManager
  commands, snapshots, or global link messages.
- `JS/BankManagerUiController.js`: JSUI paint, click, and scroll handling.
- `Max/Shared/Runtime/LinkRevisionTracker.js`: reusable monotonic revision
  tracking for global link update and operation streams.

The public atom protocol remains defined in `AGENTS.md`. New BankManager logic
belongs in the smallest component that owns its responsibility; do not add new
Max callbacks or Live API calls to the entrypoint unless they are wiring only.
