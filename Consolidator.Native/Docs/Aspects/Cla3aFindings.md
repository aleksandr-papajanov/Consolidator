# CLA-3A findings

Current VST3 uses Normal Reduction/Output controls. Reference observations below
belong to the retained Emulation mode. See [control modes](CompressorControlModes.md).

Current algorithm and final manual figures: [Cla3aProcessor](Cla3a.md).
Removed proposals are not runtime options.

- Original reference identification swept only Gain/Peak Reduction. Compressor mode
  and instantiated reference Mix/Trim/HiFreq/Analog defaults stayed fixed.
  Gain follows about 50 dB per normalized
  unit, unity near 0.408. Gain zero is attenuation, not mute.
- Gain/Peak Reduction are restored. Gain uses 50*(G-0.408) dB; Peak Reduction
  restores the cubic law in (P-0.6)/0.4 with coefficients -27.37/-22.94/14.42/-5.13.
  The exact simplified snapshot was recovered from the saved conversation after
  committed history proved insufficient. Previous Input/Output and Reduction/Output
  comparisons are historical. Optical, adaptive-release and colour math remain
  simplified; the restored control law is not the temporary linear Reduction taper.
- Recovery depends on signal level, reduction setting and preceding history.
  Two reduction states with target-dependent release give a useful approximation.
  Stronger detector smoothing and coupling the states did not solve bass recovery
  uniformly and were removed.
- Quiet output colour has an infrasonic cut and a small HF rise. The reference
  below 10 Hz differs across rates. Its nonlinear bass colour was measurable,
  but added little overall and was removed during requested simplification.
- At 24.2 Hz/0 dBFS/reduction 0.605, a preceding model's peak error was +1.03 dB
  while fundamental/RMS error was +0.37 dB. Waveform peaks conflate level with
  distortion/phase. For 59.2 Hz steps, use multi-period windows instead of 1 ms peaks.
- HF compression residual changes sign with input level. At 15.6 kHz/reduction
  0.605 the preceding model compressed about 0.31 dB too little at -20 dBFS and
  2.01 dB too much at 0 dBFS after neutral-baseline subtraction. A fixed detector
  EQ cannot resolve both points. Precise extreme-frequency matching was discontinued.
- A 199 Hz pump with quiet 997/6001/12007/15587 Hz probes did not support a large
  HF bypass controlled solely by gain reduction. At about 18 dB compression,
  reference attenuation at 997/15587 Hz differed by only 0.009 dB. Grid relative
  tilt stayed below 0.074 dB; lowering probes by 6 dB changed it by at most 0.0022 dB.
  Level-dependent detection or nonlinear strong-HF-carrier processing remain possible
  causes, not identified circuitry.

Raw inputs stay in EmulationLab/RawAudio. General workflow:
[EmulationLab](../../../EmulationLab/Docs/Aspects/Usage.md). DSP test suites remain prohibited by [rules](../Rules.md).

## Reference controls restored from saved conversation

The final simplified Gain/Peak Reduction law was recovered from the local saved
conversation instead of guessing a replacement from the later Reduction taper.
Live Waves CLA-3A metadata confirms both knobs display 0..10 linearly from
normalized 0..1 (1001 positions each). Candidate defaults read back Gain 5.50 /
Peak Reduction 4.00. Host IDs and displayed ranges are restored, with no preset
migration. Native and Release VST3 were built/installed. Current simplified
optical, release, filter and caching math is retained; no new audio fit or DSP
automated test was performed during this restoration.

## Normal makeup calibration

Fixed -18 dBFS peak / 1 kHz reference compensation was added only to Normal.
The existing detector/timing/colour laws remain; original Emulation gain is unchanged.
[Manual measurement grid, fitted law and limits](CompressorControlModes.md#normal-automatic-makeup).

## Musical-source makeup supersedes the sine calibration

The earlier -18 dBFS Normal makeup is historical and has been removed. Current
[program calibration](ProgramMakeup.md) uses eight original
recordings equally weighted. Emulation keeps its reference gain laws.
