# DeviceCompressorProcessor

[Native model composition](../../Audio/Devices/CompressorDevice.cpp),
[VST3 adapter and editor](../../../ModelPlugins.Vst3/Source/DeviceCompressorPluginProcessor.cpp).

One Normal-mode compressor selects CLA-76, CLA-3A, CLA-2A or SSL. Algorithm is
discrete, not a morph or audio blend. This composite calls the existing Native
processors; it contains no copied detector, dynamics or colour algorithm.
It is a standalone research adapter over the same Native Audio algorithms. The
main strip independently uses Audio/Devices/CompressorDevice with Shaper, detector
EQ and Managed controls/profiles. This adapter does not own the main ABI.

| Control | Domain | Default |
| --- | --- | --- |
| Algorithm | 0 CLA-76, 1 CLA-3A, 2 CLA-2A, 3 SSL | CLA-76 |
| Reduction | 0…1, linear sensitivity law | 0.5 |
| Output | Native nominal amplitude; VST3 −24…+24 dB | 1 / 0 dB |
| Attack | 0…1, short to long | 2/3 |
| Release | 0…1, short to long | 0.5 |
| Ratio | 0…1, low to high | 0.4 (nominal 4:1) |

CLA-2A/3A ignore Attack/Release/Ratio; the editor disables these controls.
CLA-76 uses all controls. SSL Attack snaps to 0 Fast or 1 Normal, both in Native
and the UI. The host parameter remains 0…1 so the same saved control works for
CLA-76; intermediate host automation is quantized for SSL. Native ignores the
unsupported controls regardless of how they arrive from the host.

## Lifecycle and switching

Prepare allocates/prepares all four models once, applies the active controls and
resets histories. SetControls stores the shared controls and updates only the
active model. Switching marks a reset pending; the next Process clears the new
model's history and snaps its ramps to the current targets before processing.
Reset clears histories and seeds Output from the current controls. Output is
manual and retains its host value across algorithm switches.
Calls belong to the same owner/audio thread as the
underlying processors; the VST3 reads host atomics at each block boundary.

Only the selected model processes audio. Switching requires no allocation, lock,
logging, I/O, reprepare, duplicate render or crossfade. Inactive histories are not
advanced. A switch is abrupt; no click-free transition is claimed. The control
positions are shared across algorithms, not separate per-device presets.

## Normal formula-based makeup

Normal uses a compact settings law fitted to eight unnormalised musical sources,
with equal source weights. It balances average loudness across algorithms rather
than preserving a particular sine operating point. Optical models depend only on
Reduction; 76/SSL also include Attack, Release and Ratio. Individual models keep
their original detector histories. Original Emulation gain is unchanged.
[Method, results and limits](ProgramMakeup.md).

Output is a manual control with the selected Native model's ordinary gain ramp.
There is no signal-dependent output matching, host Output publication or Auto Gain parameter.

## VST3

Consolidator Compressor replaces the four individual compressor VST3 targets.
IDs: algorithm/reduction/output/attack/release/ratio/bypass. The editor uses a
four-position Algorithm slider and shows normalized controls. State restoration
requires every current control to be present and finite, with no old-plugin
migration. Bypass passes raw audio and freezes DSP history. The adapter owns
host state/UI and preallocated conversion.

Native Emulation remains available on each underlying processor for future use;
this composite and current VST3 use Normal only, so the processors' Emulation-only
audio colour filters are not active. [Control modes](CompressorControlModes.md).
[Research and verification notes](DeviceCompressorFindings.md).

## Normal ratio extension

CLA-76 and SSL share the rational taper p=3*Ratio/(64-61*Ratio), R=1+99*p;
default 4:1 is at Ratio=0.4.
Below 4:1 the 76 uses slope=1-1/R with its existing 4:1 timing/knee and no ALL
contribution. At 1:1 compression and its static makeup are zero. Above 4:1 the old
4:1-to-ALL morph uses amount=(R-4)/96; this upper coordinate is not a promise of
physical 4..100:1 compression. SSL uses the same taper into its existing physical ratio law.
