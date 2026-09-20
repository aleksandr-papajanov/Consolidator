# Revival Shimmer emulation

## Scope

`PolishAirProcessor` models the Shimmer control of the Revival module hosted in
Slate Digital Virtual Mix Rack. The implementation is native realtime DSP; the
reference measurement and NLopt fitting workflow remains offline in
`EmulationLab`.

The reference rack is measured with the Revival slot soloed, Thickness at zero
and the VST parameter for Shimmer at `0`, `10`, `20`, `30` and `40`. Those
positions map to normalized Air amounts `0`, `0.25`, `0.5`, `0.75` and `1`.
Slot solo is essential: measuring a saved multi-module rack without `E-Solo`
attributes the other active modules' coloration and distortion to Revival.
The zero measurement is the rack baseline. Its complex fundamental response is
divided out for fitting the Shimmer-dependent branches.
Baseline harmonic power is subtracted per order and THD is recomputed from the
residual; otherwise VMR's neutral numerical floor is incorrectly optimized as
part of Shimmer.

## Measured behavior and model

At low input level, maximum Shimmer contributes about -1.1 dB near 1 kHz and
rises to about +11 dB at 18 kHz. The absolute module path also contains an
approximately 5 Hz first-order high-pass. The response has positive phase lead
and is better represented by parallel high-pass additions than by a
conventional high-shelf biquad.

The dominant in-band distortion is H3. Between 1 and 6 kHz it rises by almost
two decibels relative to the fundamental for every decibel of additional input,
which identifies a cubic source rather than a level-normalized exciter. H2 is
much quieter and becomes measurable mainly when its output lies in the top
octave. The tonal lift and harmonic generation therefore use separate paths:

```text
input -> direct + three HP additions + bell contour ------+-> amount curve -> add to dry
      -> level-dependent HP³ + broad bell ----------------+
      -> HP source -> odd polynomial -> HP² output --------+
      -> square -> HP⁴ top octave -> even gain ------------+
      -> 5 Hz baseline HP blended on module activation ----+
```

The level curve has anchors at `-30`, `-18`, `-12`, `-6` and `0` dBFS. It drives
a separate upper-frequency contour instead of multiplying the entire linear
response: the reference keeps almost the same response through 4–6 kHz while
its extreme-high lift contracts at loud levels. The nonlinear branch is not
multiplied by that curve. Its odd polynomial supplies the measured H3, H5 and
H7 orders, while the quieter square branch supplies H2.

All filters, peak tracking and nonlinear state are per channel. The linked peak
detector only chooses the linear level gain. Processing allocates no memory,
uses no locks and performs no coefficient compilation on the audio thread.
Zero amount adds exactly zero.

## Offline fitting

`Consolidator Air Model` exposes the Air tuning as normalized VST3
parameters. An Emulation Lab manifest selects the relevant `air.*` parameters,
captures the same conditions from Virtual Mix Rack and the candidate model, and
fits them through the generic staged optimizer.

Example:

```powershell
EmulationLab.exe capture reference manifest.json reference-full.json
EmulationLab.exe split manifest.json reference-full.json
EmulationLab.exe fit manifest.json revival-shimmer-fit.json
```

Manifest conditions represent Shimmer positions `0`, `10`, `20`, `30` and
`40`; each condition applies the corresponding textual VMR control value and
candidate Air amount. The optimizer writes a checkpoint every 25 evaluations.
A result can seed a new search without bypassing global exploration. The final
candidate is automatically captured and compared with the manifest's separate
validation dataset.
Air fitting defaults to the reference's 250 ms windows, at least 12 pre-roll
cycles and 20 measured cycles. Short four-cycle windows are unsuitable here:
Hann-window leakage appears near -90 dBc at low probe frequencies and can be
mistaken for Shimmer harmonics. Harmonic and THD comparisons ignore points where
both sides are below the configured `-100 dBc` significance floor.

The calibrated Shimmer-dependent model was refined against the complete
300-point solo reference grid. On that normalized grid it reaches 0.50 dB
fundamental-gain RMSE and 3.47 degrees phase RMSE. Significant harmonics reach
4.61 dB RMSE and THD reaches 4.28 dB RMSE; these figures exclude values below
the significance floor. The fit and per-probe report are stored in
`Measurements/revival-shimmer-solo-full-grid-fit.json` and
`Measurements/revival-shimmer-solo-full-grid-comparison.csv`.

## Validation boundary

The current reference grid covers settled mono sine probes at 44.1 kHz. A fit
must still be validated with a fresh dense sweep and held-out input levels.
Transients, stereo linking, intermodulation, aliasing, automation and other
sample rates require separate probes before the model can be treated as a
complete behavioral match.
