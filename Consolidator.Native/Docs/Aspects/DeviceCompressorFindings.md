# Universal compressor notes

These notes include measurements and integration observations from earlier
research-adapter versions. They do not validate later revisions of the former shared Native AutoGain
integration or the updated Normal signal path, which now skips Emulation-only
output colour filters. See [the current contract](DeviceCompressor.md#vst3).

This is composition of four accepted Native models, not a new reference fit.
Detector, timing and colour laws remain in each model's existing folder. Normal
Reduction/Output controls are shared. Normal uses musical-source static makeup;
Emulation retains reference gain laws. SSL attack is restricted to its two
identified endpoints.

Source review covers preparation, block-boundary selection, reset of the selected
history, ignored optical timing/ratio controls, UI disable/step state and host
persistence. DSP automated tests are not introduced; later manual measurements
are described below and in the linked research documents.
Abrupt device changes and musical listening have not been evaluated in this task.

The four old adapter sources and CMake targets are removed. Removing their old
installed bundles from C:/Program Files/Common Files/VST3 was blocked by command
execution policy, so those files may still appear in a host's plugin browser.

Native Release|x64 and ConsolidatorCompressor_VST3 Release built successfully and
installed the external/new plugin. The installed VST3 was inspected with EmulationLab
`plugin describe` and `plugin parameter-manifest`: Algorithm, Reduction, Output,
Attack, Release, Ratio, Bypass are present in that order. Defaults are CLA-76,
0.5, 0 dB, 2/3, 0.5, 0, Off. No new audio renders or GUI automation were run.

Source review found and fixed a display issue when returning from SSL to CLA-76:
after restoring the continuous Attack range, the editor rereads the unchanged host
parameter rather than retaining SSL's snapped display value. SSL's obsolete
intermediate-attack path and ramp were removed; both modes now select the original
prepared Fast/Normal coefficients immediately.

Normal makeup was subsequently calibrated and manually measured for CLA-76/3A/2A
on a -18 dBFS / 1 kHz operating point. [Results and limits](CompressorControlModes.md#normal-automatic-makeup).

SSL Normal subsequently switched to the same -18 dBFS nominal operating point,
evaluating its cached rational curve. Original SSL makeup remains only in Emulation.
[SSL measurement notes](SslChannelFindings.md#shared-normal-nominal-makeup).

## Musical-source makeup supersedes the sine calibration

The earlier -18 dBFS Normal makeup is historical and has been removed. Current
[program calibration](ProgramMakeup.md) uses eight original
recordings equally weighted. Emulation keeps its reference gain laws.
