# Consolidator.Native

Native C++ workspace for the Consolidator audio processor.

## Purpose

This workspace contains:

- **ConsolidatorCore**: a pure C++20 static library with all DSP, analysis, and
  domain logic. It has no dependency on Max, Min API, or Ableton Live.
- **ConsolidatorMax**: a Min API external that adapts Max audio and messages to
  the Core library.
- **Tests**: unit and integration tests for Core and DSP layers.
- **Tools/BuildScripts**: CI-friendly build and deployment scripts.

## Separation

| Layer            | Depends on                  | Does NOT depend on            |
| ---------------- | --------------------------- | ----------------------------- |
| Core             | C++20 standard library      | Max, Min API, atoms, messages |
| Dsp              | Core                        | Max                           |
| Analysis         | Core                        | Max                           |
| Optimization     | Core                        | Max                           |
| Max (external)   | Core, Min API               | (is the topmost layer)        |

## Audio Routing

```text
Main input       [2 ch] ─┐
Reference input  [2 ch] ─┤
                          ├─ Core Process() ── Main output      [2 ch]
                          │                    Reference output [2 ch]
                          └─────────────────── passthrough (current skeleton)
```

Future: UI controllers, analyzer outputs, monitoring, and debug streams will be
added as additional Max inlets/outlets, but Core will remain unaware of Max.

## State Routing

The Core routing scope is split by responsibility:

- `CommandRouter` dispatches state commands and chooses read versus write.
- `GroupGraph` exposes direct group members, grouped banks per instance, and
  transitive connected-group traversal.
- `StateRouter` selects source banks, resolves direct versus connected targets,
  collapses instance-owned writes, and retargets paths for target banks.
- `ParameterConstraintResolver` validates, translates, and enriches parameter limits.
- `StateWriter` applies writes as one flow: state, DSP updates, constraint refresh,
  and the final response.

For a write, `StateWriter` handles each entry explicitly: `NotHandled` allows
the caller to reject the entry, `Rejected` is reported without side effects,
`Unchanged` is returned without a DSP update, and `Applied` also publishes the
runtime update and refreshes dependent constraints.

Entries in one `WriteStateCommand` are processed independently. A later rejected
entry does not roll back earlier applied entries; write batches are intentionally
not all-or-nothing.

## CMake Targets

- `ConsolidatorCore` — static library (always built)
- `ConsolidatorMax` — Min external module (built when `C74_MIN_API_DIR` is set)
- Focused test executables grouped by CTest labels: `component`, `command`, and
  `integration`. See `Tests/README.md` for the suite map and conventions.

## Build Process

```bash
cmake -B build -DC74_MIN_API_DIR=<path/to/min-api>
cmake --build build --config RelWithDebInfo
```

When `C74_MIN_API_DIR` is omitted only `ConsolidatorCore` and tests are built.

## External Deployment

The build script at `Tools/BuildScripts/build.cmd` configures, builds, and
copies `ConsolidatorMax.mxe64` to the configured Max externals directory.

## Tools Folder

`Tools/` contains standalone CLI and dev-utility projects that consume
ConsolidatorCore directly. They are independent of Max and Ableton Live.

- `BuildScripts/` — CI and one-shot build helpers
- `Deployment/` — packaging and installer scripts
- `DevUtilities/` — profiling, test-data generation, inspection tools

## Current Stage

Architecture skeleton with passthrough DSP. All infrastructure is wired but no
processing, analysis, or fitting is implemented yet.
