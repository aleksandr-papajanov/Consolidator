# SSLChannel findings

Current VST3 uses Normal Reduction/Output controls. Reference observations below
belong to the retained Emulation mode. See [control modes](CompressorControlModes.md).

The simplified model was accepted by the user. [Current algorithm](SslChannel.md).
Removed hypotheses are not runtime options.

- Identification covered 43 settings at 44.1 kHz / block 128: Ratio 1/2/4/8/20/Inf,
  Threshold -20/-10/0 dBu, both Attack modes, Release 0.4; additional 4:1/-10 dBu
  cases used Release 0.1/1/4. Reference non-compressor controls stayed at Full Reset.
- Fast Attack changes steady suppression as well as attack speed; Release also
  changes the plateau. One fixed transfer curve with different timing misses this.
- A rational onset fitted individual transfer curves more closely than the rejected
  exponential onset. Low-order continuous functions replace interpolation anchors.
- Automatic makeup depends on Ratio/Threshold and has a stronger Threshold slope
  below -10 dBu. The effective compression onset needs measured calibration rather
  than blindly translating threshold by the nominal 18 dB alignment.
- Two histories with separate positive release laws/time floors reproduce short
  and long recovery. The first proposal was too quick at extreme Release settings.
  Weak charge-depth/fraction terms were not retained. No harmonic shaper was added.

## Accepted reference deviations

| Observation | Error versus reference |
| --- | --- |
| Stationary 998.6 Hz gain, 387 readings | 0.159 dB RMS; 0.714 dB maximum |
| Step envelope, 4257 windows | 0.298 dB RMS; 3.128 dB maximum |
| Frequency-pair levels, 516 readings | 0.682 dB RMS; 2.696 dB maximum |

Envelope uses 3 ms sine/cosine projection with a DC intercept, not instantaneous
peak or a timing offset in ms. The largest envelope residual is recovery at
Release 1 / Fast after strong prior reduction. Frequency pairs use short histories
and are not settled transfer measurements. Normal auto-attack is approximated;
very low/high carriers, other sample rates, stereo asymmetry and automation were
not matched exhaustively. The reference's tiny neutral colour is intentionally
omitted. These requested manual observations are not DSP regression thresholds.

Raw probe: EmulationLab/RawAudio/Probes/ssl-channel-level-timing.wav, 86 seconds,
continuous-phase stereo sine, stationary levels, three histories and frequency
pairs. [EmulationLab workflow](../../../EmulationLab/Docs/Aspects/Usage.md).

## Final optimization

The final Native/VST3 build was compared with the accepted candidate on all 43
static settings using the same complete 86-second stereo input. Every output
float32 sample matched; maximum absolute sample difference and relative waveform
RMS difference were both zero. This is a requested manual comparison, not a DSP
test suite or a guarantee for all inputs/automation/sample rates.

Optimization separates curve, makeup and timing dependencies; removes the zero
slope feature; folds the fixed RMS offset into onset; prepares release retention;
and uses exp rather than general pow for audio attenuation. CPU speedup was not
benchmarked. Source review found no ownership/realtime or duplicated-DSP issues.

## Shared Normal nominal makeup

Normal now estimates expected reduction at -18 dBFS peak through the current
rational curve, matching the operating point chosen for the other three models.
The former makeup polynomial is used only in Emulation. No detector or timing
coefficients changed. Makeup is cached with slope/bend/onset and uses no new
measurement state or per-sample fitting.

Manual EmulationLab render-grid: 120 combinations of five Reduction values,
four physical ratios (1, 4, 20, limiting), three Release positions and both Attack
endpoints. Stereo 44.1 kHz / block 128; 20-second 1 kHz sine at -18 dBFS peak,
fundamental measured over the last second. Relative to each matching Reduction=0
baseline, residual gain error is 0.113 dB RMS / 0.347 dB maximum. These observations
include zero-compression conditions; other frequencies, sample rates and musical
loudness were not evaluated. Native and Release universal VST3 built and installed.
Source review found no added allocations, locks, detector feedback or duplicate DSP.
No DSP automated tests were created or run.

## Musical-source makeup supersedes the sine calibration

The earlier -18 dBFS Normal makeup is historical and has been removed. Current
[program calibration](ProgramMakeup.md) uses eight original
recordings equally weighted. Emulation keeps its reference gain laws.
