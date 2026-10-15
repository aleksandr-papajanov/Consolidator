# Target observation protocol

`observe_banks` receives
`targetInstanceId` and a strict processor context, then returns one
`bank_state_snapshots` frame containing each bank's relative equalizer paths
and values. It does not change the focused target.

`initialize` returns the external's managed instance ID and transient UI context.
The default context is `equalizer`. `observe_target` receives
`targetInstanceId`, `bankId` and one strict context symbol (`gain_staging`, `saturator`,
`compressor`, `equalizer` or `polish`), then atomically records the UI context
and target before returning one `target_state_snapshot` frame containing the
target identity, bank, context, entry count, and each relative path
with its value and physical/effective ranges. A successful frame implies
`ready` for every entry; an error applies to the complete frame.
The bank manager automatically selects `group` edit scope when focus moves to a
grouped bank and selects `local` scope when focus moves to an ungrouped bank.
The scope toggle remains available after that automatic selection.
The context is per-instance transient UI state: it is not a `StateValue`, does
not participate in history or persistence, and never affects DSP. Later changes use
`state_changed` with the same semantic paths and range metadata.

Target selection suspends bindings before sending
`observe_target`, then replaces the target cache and resumes after the complete
snapshot. Instance activation keeps bindings inactive until this same target
transition completes.

Bank addressing: [BankIndexContract](BankIndexContract.md).
Frontend assembly: [SnapshotAssembly](../../../Consolidator.Vst3/Docs/Aspects/SnapshotAssembly.md).
