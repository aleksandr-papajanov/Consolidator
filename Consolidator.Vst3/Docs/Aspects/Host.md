# Windows VST3 host

Windows x64 JUCE host for the main Consolidator device. Application ownership
is defined in [Rules](../Rules.md), presentation structure in
[FrontendComposition](FrontendComposition.md). Host automation, IPC, macOS and independent
simultaneous viewers are outside the current scope. The JUCE revision is 8.0.4;
manufacturer `Cnsl`, plugin code `CsDv`, bundle ID `com.consolidator.plugin`.
`ModelPlugins.Vst3` is a separate research-adapter scope.
## Audio and ownership

Each `PluginProcessor` owns its Managed instance ID, opaque audio handle,
`DspStateExchange`, `DspRuntime` and preallocated double scratch buffers. The
main bus is stereo in/out. Float processing converts through those buffers;
double processing calls the same graph directly. Oversized host blocks are
processed in chunks no larger than the prepared capacity, without resizing on
the audio thread. Analysis receives the dry stereo pair before graph processing,
VST3 additionally exposes an optional stereo
Sidechain input for the display-only reference spectrum. Its Managed capture,
worker FFT and shared notification delivery are described in
[SidechainReference](SidechainReference.md); it never enters the DSP
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
are in [Gestures](Gestures.md).

## State and viewer lifecycle

`getStateInformation` captures a committed UTF-8 Managed snapshot through the
existing FIFO barrier. `setStateInformation` restores through the same strict
Managed validator and baseline transaction. Raw bytes are stored in the VST3
state; there is no duplicate XML state tree, schema
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
active while focus is outside plugin editors. 
Host-provided track names are sent through the existing instance-label write;
availability depends on the host.

Coordination stays process-local. Session isolation and communication between
plugin-host processes are not added. A preset restores group IDs with the
existing Managed semantics. The VST3 host does not reinterpret those IDs.
## Packaging

Resolve `Consolidator.Managed.dll` beside the actual VST3 module, not beside the DAW executable. The shared bridge uses wide
Windows paths and requires `nlopt.dll` beside it. Missing libraries/exports
produce diagnostics and disable audio if registration fails.

Visual Studio Publish, runtime output, CMake presets and installation have one
source: [Build](Build.md). Build/installation do not establish Live lifecycle
or UI correctness; validate the changed host behavior separately when authorized.