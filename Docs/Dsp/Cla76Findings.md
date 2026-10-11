# CLA-76 useful research findings

Research is complete. [Cla76Processor](Cla76Processor.md) describes the current
algorithm; old datasets, manifests, scores and intermediate revisions are not
the production specification. Raw inputs remain in EmulationLab/RawAudio.

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
  removed; nominal Input/Output replaced hardware attenuator emulation by choice.
- Reference parameter text parsing was unreliable. Verified WaveShell input
  coordinates were -48 dB at 0.125, -30 dB at 0.375 and -18 dB at 0.625.
  Do not extrapolate the low dial from the default coordinate. Bluey=0,
  Analog Off=1, Trim=0.5, Mix=1; reference Ratio=0.75 selects 4 and 1 selects ALL.
  Reference knob orientation is 1 slow, 7 fast; current model amounts increase
  with time and map to reference coordinate 7-6*amount.
- Continuous carrier phase matters: a phase reset at a level step can dominate
  the first peak. Compare waveform shape after removing a scalar level offset,
  and report the level separately. Cold start and continuous history differ.

Exact full-band, full-level and musical equivalence is not claimed. Past numerical
scores preceded nominal gain controls and removal of research overrides, so they
are not current accuracy guarantees. See [EmulationLab](EmulationLab.md) for future
manual work and [DSP rules](Rules.md) for the prohibited automated test scope.
