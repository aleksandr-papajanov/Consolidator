# Filter catalog

Managed owns the filter catalog used to construct state. It is the single
source for filter kind, available parameters and initial values. The Max UI
mirrors only the presentation capabilities needed to bind controls.

The legacy standard equalizer layout was:

```text
Gain, Tilt, LowShelf, HighShelf, Bell, Bell, Bell
```

The detector layout for both compressor and saturator was:

```text
LowShelf, Bell
```

All frequency-capable filters use `20..20000 Hz` and `-24..24 dB` gain.
Editable Q is available only on Bell filters and uses `0.1..10`, default
`0.707`. Shelf and Tilt filters use fixed Q `0.707`; it is not an editable
state parameter. Their default frequencies are `1000, 100, 10000, 1000, 2000,
4000 Hz` for the frequency-capable EQ slots and `100, 1000 Hz` for detectors.
Gain filters expose only gain. The `filter_catalog` and analyzer state frames
carry the filter kind, fixed Q and named parameter values explicitly. The UI
does not infer a filter type from its slot index.
