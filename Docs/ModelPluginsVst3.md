# Consolidator model VST3 plugins

`ModelPlugins.Vst3` builds independent offline model adapters for production
Native DSP processors. It currently produces `Consolidator Thick Model`,
`Consolidator Air Model` and `Consolidator Transient Shaper Model`. Plugin
Doctor and `EmulationLab` can host each adapter independently.

The Polish processing boundary is represented by two plugins, not one combined
candidate. `Consolidator Thick Model` exposes Thick, Bypass and only
`PolishThickTuning`; `Consolidator Air Model` exposes Air, Bypass and only
`PolishAirTuning`. Each has its own VST3 class ID, state, parameter manifest,
experiment and optimization run. Tuning values are read only by
`prepareToPlay`; `EmulationLab` applies a complete candidate and then reprepares
the plugin before rendering. No coefficient compilation, allocation or
optimizer work is introduced into the audio callback.

The product name, bundle identifier and VST3 class code differ from the removed
`Polish Test 2` adapter so a host cannot reuse its cached module. Generate a
manifest parameter list with `EmulationLab plugin parameter-manifest`, select
the relevant coefficients in optimization stages, and keep the plugin's Amount
parameter as a condition control rather than a fitted topology value.

The Transient Shaper adapter exposes Attack, Sustain and Bypass plus detector,
envelope, gain-response, scale, ceiling and gate coefficients from
`TransientShaperTuning`, including its slow autogain detector, correction,
ratio, limit and learning floor. Attack and Sustain are condition controls. The
tuning is applied during `prepareToPlay`; the audio callback only updates the
two smoothed amount parameters and runs the production processor.

The adapter remains separate from the Max external and calls the native
production processors directly. Each therefore tests one DSP boundary without
the rest of `DspRuntimeGraph`. A new emulation target should add another plugin
target and thin JUCE adapter here rather than create another standalone project.
