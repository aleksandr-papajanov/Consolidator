# CLA-3A findings

Current algorithm and final manual figures: [Cla3aProcessor](README.md).
Removed proposals are not runtime options.

- Original reference identification swept only Gain/Peak Reduction. Compressor mode
  and instantiated reference Mix/Trim/HiFreq/Analog defaults stayed fixed.
  Gain follows about 50 dB per normalized
  unit, unity near 0.408. Gain zero is attenuation, not mute.
- Current controls are detector-only Input and audio Output with -infinity..0 dB
  attenuation display, amplitude 0..1 and Native +16/+30 dB calibration, at a fixed
  -25.7 dB threshold. Static conversion preserves the old over-threshold signal
  and optical response; 15 manual waveform comparisons differed by at most
  0.000047% relative RMS. Automation equivalence was explicitly not required.
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
[EmulationLab](../../../../../Docs/Dsp/EmulationLab.md). DSP test suites remain prohibited by [rules](../../../../../Docs/Dsp/Rules.md).
