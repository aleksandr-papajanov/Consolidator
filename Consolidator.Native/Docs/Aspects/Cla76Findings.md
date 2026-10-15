# CLA-76 useful research findings

Current VST3 uses Normal Reduction/Output controls. Reference observations below
belong to the retained Emulation mode. See [control modes](CompressorControlModes.md).

Research is complete. [Cla76Processor](Cla76.md) describes the current
algorithm; old datasets, manifests, scores and intermediate revisions are not
the production specification. Raw inputs remain in EmulationLab/RawAudio.
The final simplification keeps the meaningful ALL dynamics and one-pole colour,
removes two corrective biquads, rounds constants and merges all audio into one
pass. Current manual comparison figures are in the processor document.

- A power detector for Attack and an amplitude detector for ordinary Release
  gave a useful fit. The gain must use preceding detector state, otherwise the
  current sample suppresses its own initial peak.
- ALL recovery depends strongly on level. Linear discharge in sqrt(amplitude)
  reproduces the accelerating shoulder more closely than one exponential time.
  This is an empirical model, not identification of the Waves circuit.
- ALL also needs bounded slow gain memory to describe constant-carrier settling.
  Capping its applied contribution by the current curve target avoids extra
  attenuation after a drop below the compression range.
- Attack and Release influence both timing and settled gain in ALL. A fast-Release
  carrier coupling plus threshold/slope functions describe this without another
  processor, audio crossfade or event timer.
- Reference LinearAnalysis depends on signal level and history. Harmonics from
  moving gain do not prove a separate saturation stage. Added saturation was
  removed. Current Input/Output restore the measured rational attenuator tapers.
  Attack/Release use the reference 1..7 orientation and retain their fitted laws.
- Reference parameter text parsing was unreliable. Verified WaveShell input
  coordinates were -48 dB at 0.125, -30 dB at 0.375 and -18 dB at 0.625.
  Do not extrapolate the low dial from the default coordinate. Bluey=0,
  Analog Off=1, Trim=0.5, Mix=1; reference Ratio=0.75 selects 4 and 1 selects ALL.
  Reference knob orientation is 1 slow, 7 fast; current model amounts increase
  with time and map to reference coordinate 7-6*amount.
- Continuous carrier phase matters: a phase reset at a level step can dominate
  the first peak. Compare waveform shape after removing a scalar level offset,
  and report the level separately. Cold start and continuous history differ.

Exact full-band, full-level and musical equivalence is not claimed. Earlier numerical
scores preceded nominal gain controls and removal of research overrides; final
simplification scores compare the current public controls. See [EmulationLab](../../../EmulationLab/Docs/Aspects/Usage.md) for future
manual work and [DSP rules](../Rules.md) for the prohibited automated test scope.

Restored reference controls and rational-function provenance are documented in
[Cla76](Cla76.md). Mathematical timing/colour optimizations remain.

## Reference controls restored from saved conversation

The previously optimized P8/Q7 attenuator functions were recovered with full
coefficient precision from the saved conversation. They replace the temporary
linear Reduction/nominal Output controls; no anchor tables or old saturation/
corrective filter branches were restored. Attack/Release are again reference
1..7, with 1 slow and 7 fast. Installed VST3 defaults read back -30/-18 dB and
3/4, Ratio 0 (the fitted 4:1 endpoint). Native and Release VST3 were built and
installed; timing/memory/one-pole colour coefficients remain the current simplified
values. No new reference-audio accuracy claim follows from this control restore.

## Normal makeup calibration

Fixed -18 dBFS peak / 1 kHz reference compensation was added only to Normal.
The existing detector/timing/colour laws remain; original Emulation gain is unchanged.
[Manual measurement grid, fitted law and limits](CompressorControlModes.md#normal-automatic-makeup).

## Historical Normal ratio extension to 1:1

Normal now shares SSL's nominal coordinate R=1+99*control and default R=4,
control=3/99. Below 4:1 the slope is 1-1/R, while timing/knee remain at the existing
4:1 endpoint and ALL contribution is zero. Above 4:1 the old morph amount is
(R-4)/96. Curve/makeup caches also track the original normalized control so changes
within 1:1..4:1 update the slope even though ALL amount stays zero.
Emulation keeps its original 4:1-to-ALL coordinate and default zero.

Native Release|x64 and the universal Release VST3 built successfully. Installation
initially hit Permission denied, then succeeded after the user closed the host.
Installed parameter readback confirms Ratio=0.0303030. Both SSL's standalone
Normal defaults and the shared controls now use this 4:1 position; SSL Emulation
retains its original 1:1 default. Native constructors initialise ramps in the
selected mode's units, including callers that Prepare without SetControls.

Manual EmulationLab investigation rendered 17 settings on the existing 20-second
-18 dBFS / 1 kHz stereo probe at 44.1 kHz / block 128. For 1:1 the complete
Reduction=0/1 output samples were identical on this probe. Nine remapped 4:1,
intermediate and ALL settings were compared with saved preceding-version renders:
maximum relative waveform RMS difference 7.01e-9; maximum sample difference
2.98e-8. These are scoped manual observations, not a DSP automated test or proof
for every signal/automation path. Source review found no new allocation, locking,
I/O or DSP duplication. This historical Normal comparison predates the filter change:
the one-pole colour now runs only in Emulation, not at Normal 1:1.

## Corrected Normal knob taper

The first linear 1..100 mapping put 4:1 at 0.0303, leaving too little travel for
ordinary ratios. It is replaced by p=3*u/(64-61*u), R=1+99*p, shared by 76/SSL.
The default is now u=0.4; endpoints remain 1:1 and the previous maximum/ALL.
The taper is applied to targets before the existing internal ramps; compression,
makeup and ALL laws retain their physical/internal coordinates. Emulation is unchanged.
Native Release|x64 and universal Release VST3 built and installed successfully;
installed parameter readback confirms Ratio=0.4000000. This correction was reviewed
by following defaults/constructor/SetControls through the internal coordinate and
the 4:1 pivot; no new audio measurements or DSP automated tests were run for the taper.

## Musical-source makeup supersedes the sine calibration

The earlier -18 dBFS Normal makeup is historical and has been removed. Current
[program calibration](ProgramMakeup.md) uses eight original
recordings equally weighted. Emulation keeps its reference gain laws.
