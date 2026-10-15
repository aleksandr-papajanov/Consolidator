# CLA-2A identification notes

Current VST3 uses Normal Reduction/Output controls. Reference observations below
belong to the retained Emulation mode. See [control modes](CompressorControlModes.md).

## Emulation controls

Reference Gain/Peak Reduction are restored. Both use normalized 0..1 values in
Native and nonlinear 0..100 host readouts; defaults display 40.00/46.00.
The rounded regional Gain law and its taper were recovered from the saved chat.
Peak Reduction threshold, optical dynamics and colour retain the current simplified
implementation. See [Cla2a](Cla2a.md). The conversion observations below are
historical and do not claim new waveform verification of the restored interface.

## Observations

- Fresh Waves CLA-2A Stereo instances report Gain 0.535 and Peak Reduction 0.598.
  These display as Gain 40.00 and Peak Reduction 46.00, not 53.5/59.8: both reference
  readouts have a nonlinear two-region taper. The previous adapter fitted it from 1001 metadata
  points per knob and used a logarithmic inverse for numeric entry. No audio coefficients were refitted for this UI correction.
  Only these controls change during the research; all other instantiated defaults
  remain fixed. The manifest records the actual normalized values.
- Gain does not behave as a linear dB knob: neutral compression measurements range
  from about −51.35 dB at zero to +38.8 dB at 100. In displayed units its response
  consists of successive affine regions. A smooth two-tanh fit and a global
  polynomial left meaningful level errors; explicit regional slopes match the
  measured control law. Both regional laws are restored for the reference controls.
- Peak Reduction up to about 0.3 causes little compression for the measured inputs
  through 0 dBFS. Higher settings chiefly lower the threshold. Gain cross-checks
  support keeping the output knob outside the detector.
- The quiet response has about 1 dB loss at 8 Hz and around 0.6 dB at high frequencies.
  A first-order high pass and one shelf reproduce its main contour.
- Recovery depends on remaining reduction as well as stored history. Using only
  stored reduction left the high-Peak-Reduction release substantially too slow.
- Equal recovery laws made the two memories converge after a long hold, losing
  the reference's slow tail and raising the next cycle's initial plateau. Separate
  recovery times and a stronger remaining-target dependence for the slow memory
  preserve quiet history without delaying recovery under continued compression.
- The initial 3A-derived 1.7 ms detector recovery also delayed the beginning of
  release at deep compression. A 0.3 ms recovery reduced the strongest early error
  from about 3.7 to 2 dB before recalibrating the complete detector/memory level.
- The middle-depth attack needed faster initial charge, while deep-reduction
  attacks needed slower charge. A positive rational modifier improved both in
  the measured grid, reducing default-setting attack error from 1.46 to 0.83 dB.

## Scope of measurements

31 control combinations, stereo 44.1 kHz, block 128. The 103-second raw input
contains four 5/4/5-second level histories, seven 3-second stationary levels
(−48…0 dBFS, 998.6 Hz), and thirteen quiet frequency probes from 8 Hz to 20 kHz.
Every condition uses a fresh plugin instance. Stationary gain uses the fundamental,
not waveform peak. Short-window timing uses sine/cosine least-squares projection;
the first milliseconds also contain detector and filter settling.

The follow-up grid uses 46 displayed Gain/Peak Reduction combinations and a
36-second raw input. It contains three 1/2/1-second cycles at −20/0/−20 dBFS,
followed by six 4-second holds from −48 to 0 dBFS. Per-render parameter readback
confirms that both sides actually receive the same displayed settings. The short
projection includes a DC intercept so filter settling does not bias amplitude.

## Remaining limits

This simplified model retains documented reference deviations. Long optical tails, the strongest initial
attacks, and the reference's changing effective ratio at deep reduction are
approximations. The former control-region slopes reproduced the reference's main response.
Very high output levels, sample-rate changes, stereo separation and automation
have not been measured against the reference in this research.

No added saturation was identified. For example the uncompressed 0 dBFS tone at
Gain 38.72 (normalized 0.5) has reference H2 about −46.7 dBc; reproducing this separate harmonic
component is outside the current fit. Detector-driven gain modulation remains
part of the compression algorithm. There is no claim of waveform identity or
musical listening verification from these tone measurements.

## Measured candidate before the control conversion

The previous interface candidate on the 46-condition short/repeated grid:

| Measurement | Deviation from reference |
| --- | --- |
| Settled gain, −24/−12/−6/0 dBFS, 184 readings | 0.047 dB RMS; 0.164 dB maximum |
| All short-window envelope points, 3864 readings | 0.166 dB RMS; 1.455 dB maximum |
| Attack windows at Gain 40 / Peak Reduction 46 | 0.351 dB RMS; 0.834 dB maximum |
| Attack windows at Gain 40 / Peak Reduction 100 | 0.366 dB RMS; 0.951 dB maximum |

On the same 184 settled-gain readings the previous candidate had 0.429 dB RMS
and 1.209 dB maximum error. Its envelope errors were 0.472 dB RMS and 3.428 dB
maximum on the same 3864 windows. This is a paired comparison of identical inputs
and displayed settings, rather than a comparison between different probe grids.

Envelope errors compare projected amplitudes in dB, not timing offsets in ms.
They include transient shape and plateau-level error. Each short projection spans
the following 3 ms, so the first point is not an instantaneous sample peak. The
−48/−36 dBFS holds are excluded from the settled-gain metric: four seconds after
strong compression still contain optical recovery, especially in the reference.
These are manual research observations, not automated regression thresholds.

At 40/46, the old hot plateau was about 0.50 dB below the reference; the repeated
low plateau was about 0.38 dB above it. The dense control/optical fit reduced those
errors to roughly 0.08 dB and 0.13 dB respectively. Cold low-level gain now
differs by about 0.004 dB. This resolves the main observed level disagreement;
remaining early transients and broader musical behaviour need listening feedback
for future changes beyond the current simplified model.

## Historical verification of the temporary Input/Output conversion

The release VST3 and Native external were rebuilt and installed. Source review
found no new allocation, locking, I/O, duplicated DSP or host-side algorithm.
The previous knob-scale APIs, gain/threshold region arrays and control caches
have been removed. Native and VST3 compile the same algorithm.

EmulationLab rendered 46 static mapped control pairs at 44.1 kHz / block 128 on
the 36-second short/repeated 998.6 Hz probe and compared whole stereo waveforms
against saved renders of the previous model. Maximum relative waveform RMS
difference was 4.1643e-7 (0.0000417%); maximum absolute sample difference relative
to the corresponding old waveform peak was 4.7068e-7 (0.0000471%). The comparison
includes repeated attack/release cycles and quiet recovery. This verifies the
control conversion on these inputs, not accuracy against Waves or automation.
No DSP automated tests were created or run. Musical listening and other sample
rates were not evaluated during this conversion.

## Verification of restored reference controls

Native Release|x64 and ConsolidatorCla2aModel_VST3 Release built successfully;
the Native external and installed VST3 were updated. EmulationLab `plugin describe`
on the installed candidate reports Gain 40.00 [0.535], Peak Reduction 46.00 [0.598]
and Bypass Off. The scale/inverse and rounded regional Gain coefficients were
recovered from the saved project chat, without a new audio fit.

Source review traced both controls through the adapter into the shared Native
processor, including preparation, reset, ramps and coefficient caches. No duplicate
DSP, allocation, locking, I/O or migration API was added. Detector, optical memory,
deep-reduction and colour coefficients are retained. No new audio comparison or
DSP automated tests were run; historical accuracy figures remain historical.

## Normal makeup calibration

Fixed -18 dBFS peak / 1 kHz reference compensation was added only to Normal.
The existing detector/timing/colour laws remain; original Emulation gain is unchanged.
[Manual measurement grid, fitted law and limits](CompressorControlModes.md#normal-automatic-makeup).

## Musical-source makeup supersedes the sine calibration

The earlier -18 dBFS Normal makeup is historical and has been removed. Current
[program calibration](ProgramMakeup.md) uses eight original
recordings equally weighted. Emulation keeps its reference gain laws.
