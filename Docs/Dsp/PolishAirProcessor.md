# PolishAirProcessor

[Исходник](../../Consolidator.Native/External/Dsp/PolishAirProcessor.cpp). Модель Revival Shimmer.



Air is the native model fitted to the measured Revival Shimmer control:

```text
input -> direct gain + three HP additions + bell contour --+-> amount^exponent -> add to dry
      -> level HP³ + level bell ----------------------------+
      -> HP source -> odd polynomial -> HP² output ---------+
      -> square -> HP⁴ top-octave output -> even gain ------+
      -> 5 Hz baseline HP blended on module activation -----+
```

The three single-pole high-pass additions and local bells reproduce the measured
complex linear response more directly than a conventional shelf. A separate
three-pole high shelf and broad bell apply the linked input-level curve only to
the upper contour: the reference's top lift contracts as a sine approaches
0 dBFS while 4–6 kHz remains substantially stable. The odd branch includes
cubic, fifth-order and seventh-order terms and stays outside that attenuation.
A much quieter square branch models the top-octave H2 residue. Amount uses a
fitted exponent so intermediate knob positions are not assumed linear in wet
amplitude.

The linked detector attacks immediately, holds for one second while recurring
peaks refresh it, and then releases over 2 seconds. The hold prevents its gain
from creating a false harmonic comb on stationary tones. It controls only the
linear additions. Filter and detector histories are
fixed per-channel state, and zero Amount is an exact identity. Detailed
measurement assumptions and the fitting command are in
[PolishAirFindings.md](PolishAirFindings.md).
