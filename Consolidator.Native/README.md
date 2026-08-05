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

## CMake Targets

- `ConsolidatorCore` — static library (always built)
- `ConsolidatorMax` — Min external module (built when `C74_MIN_API_DIR` is set)
- `ConsolidatorCoreTests` — unit test executable

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