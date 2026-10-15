# Windows VST3 migration

## Scope and decisions

Replace the main Max frontend with a Windows x64 JUCE VST3 frontend. Keep
`Consolidator.Managed` as the state/analysis owner and reuse the existing Native DSP graph and C
ABI. Host automation, IPC, macOS and independent simultaneous viewers are out
of scope. No JUCE parameter tree is introduced as a second authoritative state
store. Managed remains responsible for values, groups, history and persistence.

The main plugin lives in `Consolidator.Vst3`; `ModelPlugins.Vst3` remains the
separate processor-emulation scope. The main plugin uses the same JUCE 8.0.4
revision. Its manufacturer code is `Cnsl`, plugin code is `CsDv`, and bundle ID
is `com.consolidator.plugin`.

## Implementation plan

1. **Host foundation — Release x64 VST3 built; host validation pending.**
   Add the Windows VST3 target, reuse the complete Native graph, adapt the
   existing Managed bridge for host-provided library paths and logging, and
   connect raw Managed persistence to VST3 state callbacks.
2. **Processor editor — initial implementation in source.**
   Six processor contexts, all seven EQ banks, scalar controls, detector/EQ
   filter controls, bypass, dedicated Leveler disable, reset and shared history.
   Continuous bounded-rate gestures, effective group ranges and final gesture
   acknowledgements are implemented in source. Existing Max UI classes define
   the behavior being ported; see [Vst3Editor.md](Vst3Editor.md). Native ring
   dials, precision, Character gestures and Input runtime overlays are connected
   in source; see [Vst3Controls.md](Vst3Controls.md). The responsive panel layout
   follows the feature-controller macro order. Visual/host verification remains pending.
3. **Bank Manager and analyzer — initial implementation in source.**
   Revisioned registry snapshots/deltas, cross-instance focus, local/group
   scope, topology selection, group/ungroup/clear, mute/solo/instance bypass,
   processor/bank bypass and targeted reset, Bank Commit/Refine and track-level
   dots are implemented. The analyzer renders local EQ/detector responses and
   existing source/group FFT spectra. Graphical filter gestures, Alt-drag Q,
   reset and continuous analyzer gestures are implemented in source. The
   all-bank response overlay is connected through the existing bank snapshots;
   dial precision and Input runtime overlays are also connected in source.
   Preserve Managed constraints rather than recreating coordination in C++.
4. **Host lifecycle validation and cutover — pending, requires permission.**
   The main plugin is built. Validate state save/load, reopen editors, duplicate
   instances, groups, shared history, sample-rate/block changes and offline
   rendering in Ableton on Windows. Do not create or run DSP automated tests.
   Remove the Max frontend only once the complete replacement has been
   implemented and the host workflow has been validated. Until then Max is the
   current frontend, not a fallback used by the VST3 plugin.

## Audio and ownership

Each `PluginProcessor` owns its Managed instance ID, opaque audio handle,
`DspStateExchange`, `DspRuntime` and preallocated double scratch buffers. The
main bus is stereo in/out. Float processing converts through those buffers;
double processing calls the same graph directly. Oversized host blocks are
processed in chunks no larger than the prepared capacity, without resizing on
the audio thread. Analysis receives the dry stereo pair before graph processing,
matching the existing Max host. VST3 additionally exposes an optional stereo
Sidechain input for the display-only reference spectrum. Its Managed capture,
worker FFT and shared notification delivery are described in
[Vst3SidechainReference.md](Vst3SidechainReference.md); it never enters the DSP
graph; both pairs enter Managed audio capture.

The host must finish audio callbacks before processor destruction. Destruction
serializes with the JUCE message thread, stops the presentation timer, removes
the editor listener and calls Managed unregister before releasing exchange,
callback context or queue storage. NativeAOT remains mapped until process exit.
The last successful bridge owner shuts down Managed services; failed bridge
construction does not count as a runtime owner.

Managed callbacks borrow ABI pointers only for the duration of the call.
Control frames are copied into a lossless native FIFO; FFT, telemetry and track
levels each have one replaceable latest frame. A 30 Hz message-thread timer
drains up to 32 control frames per tick and delivers frames to the current
editor. With no editor, presentation frames are not copied or accumulated.
Persistence dirty notifications remain active even without an editor and call
JUCE's non-parameter-state host notification from the message thread. Callback
exceptions become atomic failure counts and are logged on that thread.

The editor sends the existing local/group commands and atomically replaces
controls only after the matching complete target snapshot. Continuous gestures
own one history point and retain preview until final acknowledgement. Dedicated
commands retain their existing Managed history ownership; Reset/Commit/Refine
do not receive a duplicate frontend history advance. No presentation-owned
default values are persisted or applied to DSP. Details and source mappings
are in [Vst3Editor.md](Vst3Editor.md).

## State and viewer lifecycle

`getStateInformation` captures a committed UTF-8 Managed snapshot through the
existing FIFO barrier. `setStateInformation` restores through the same strict
Managed validator and baseline transaction. Raw bytes are stored in the VST3
state; there is no Max numeric Blob encoding, XML copy of DSP state, schema
adapter or legacy decoder. Restore does not require an editor or message-thread
dispatch; an open editor refreshes on the next presentation tick. Capture and
restore are control operations, not audio-processing calls.

Opening the editor initializes the existing protocol. Once its Windows host
window gains foreground focus, it activates its Managed viewer and requests
registry and current target snapshots. Closing it deactivates that
viewer. Windows foreground focus selects an already-open editor automatically: Managed
still supports one active viewer globally. Other open editors are not
independent viewers; activating another JUCE window disables the previous
window's editing. There is no manual Activate button. The last viewer stays
active while focus is outside plugin editors. Live selected-track observation is not carried over.
Host-provided track names are sent through the existing instance-label write;
availability depends on the host.

Coordination stays process-local. Session isolation and communication between
plugin-host processes are not added. A preset restores group IDs with the
existing Managed semantics. The VST3 host does not reinterpret those IDs.
Running Max and VST3 against a shared loaded NativeAOT module at the same time
is not a supported cutover workflow: each frontend owns its runtime lifetime.

## Packaging and build

The host resolves `Consolidator.Managed.dll` beside the actual VST3 module,
not beside the DAW executable and not through the Max search path. The shared
bridge uses wide Windows paths and requires `nlopt.dll` in the same directory.
Missing libraries/exports produce diagnostics; the plugin disables audio when
registration fails rather than pretending that it initialized successfully.

CMake requires `CONSOLIDATOR_MANAGED_RUNTIME_DIR` to contain both required
DLLs. The Visual Studio preset at `Consolidator.Vst3/CMakePresets.json` points
to `Consolidator.Vst3/build/managed-runtime`; the single `Vst3Plugin` Publish
profile writes the Managed runtime directly to that directory. The
`ConsolidatorVST3Install`
target copies the completed bundle, including both runtime DLLs, into
`CONSOLIDATOR_VST3_INSTALL_DIR`. The default is the standard Windows x64 VST3
directory under `CommonProgramW6432`; override that cache path when configuring
for a different host location.

`Consolidator.Vst3.vcxproj` is included in the main root solution and delegates
its build to the CMake presets. The Managed project has one Visual Studio
Publish profile, `Vst3Plugin`. Publishing it writes the NativeAOT library and
its dependencies directly to `Consolidator.Vst3/build/managed-runtime`, builds
the VST3 and installs the bundle and runtime DLLs to the configured system
VST3 directory. This workflow does not build the transitional Max Native
external or maintain a separate Managed publish output directory.

The Visual Studio makefile project resolves `cmake.exe` from the Visual Studio
installation using `VCTargetsPath`; it does not assume that the IDE process has
CMake on `PATH`. The `Vst3Plugin` Publish profile invokes this project through
Visual Studio MSBuild after publishing Managed.

VS Code's CMake Tools source and configure preset also point to this VST3
project, keeping its generated files under `Consolidator.Vst3/build`.

To build from the VST3 project directory in Developer PowerShell:

```powershell
cmake --preset vs2026-x64
cmake --build --preset vs2026-release-install
```

The first configure fetches JUCE unless its FetchContent source is supplied
externally. Publishing with `Vst3Plugin` performs the complete Managed-to-VST3
workflow. A failed copy to the selected VST3 directory fails the install
target rather than leaving a successful-looking publish.

## Validation status

The organized frontend refactor described in
[Vst3Architecture.md](Vst3Architecture.md) was built successfully in Release x64
on 2026-10-13 with no warnings in the build output. The installed Windows VST3
bundle was updated and its module hash matched the built artifact. This build
reused the configured published runtime without building or publishing Managed.
Clients, bindings, feature controllers and host composition have been reviewed
in source; the refactored UI still requires a Live pass. See that document for
the exact command and layer ownership.

On 2026-10-13, the former `Vst3Plugin` Visual Studio publish profile completed
successfully. At that time it published the Managed runtime, built VST3 and
installed the bundle, but it did not build the Max Native project. That
single-profile workflow is restored as the current `Vst3Plugin` profile.

Ableton Live was not launched and has not rescanned or loaded the plugin.
No audio/DSP tests or host lifecycle validation were run. The plugin is
installed for host testing, not yet validated as a replacement for the Max
frontend. Persistence and UI/lifecycle behavior still require verification in
Live without adding automation parameters.

On 2026-10-14, a Visual Studio publish successfully published Managed but its
nested VST3 build failed with exit code 9009 because `cmake` was not on the
IDE process `PATH`. The makefile project's build, rebuild and clean commands
now invoke the Visual Studio-bundled CMake executable explicitly. The build
profiles have since been separated; the CMake path fix has not yet been
verified by a new VST3 build.
