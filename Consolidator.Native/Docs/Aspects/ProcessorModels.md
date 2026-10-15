# Shared processor models and configuration

These seven simplified models are complete, with a universal compressor composite. Four compressors default to Normal
Reduction/Output; reference control laws are retained in explicit Native Emulation.
[Control modes](CompressorControlModes.md). Production sources are in
`Consolidator.Native/External/Dsp/Processors/<Processor>`; their documents are in
`Consolidator.Native/Docs/Aspects/<Processor>.md`. C++ namespace:
`consolidator::max::dsp::processors`.

| Folder | Algorithm | Research notes |
| --- | --- | --- |
| [DeviceCompressor](DeviceCompressor.md) | Shared selection of four Normal compressors | [Findings](DeviceCompressorFindings.md) |
| [Cla76](Cla76.md) | FET compressor, continuous 4:1 to ALL | [Findings](Cla76Findings.md) |
| [Cla2a](Cla2a.md) | Optical compressor, Reduction/Output with retained reference mode | [Findings](Cla2aFindings.md) |
| [SslChannel](SslChannel.md) | Channel compressor, Reduction/Output, automatic makeup and normalized timing/ratio | [Findings](SslChannelFindings.md) |
| [Cla3a](Cla3a.md) | Optical compressor, reference Gain and Peak Reduction | [Findings](Cla3aFindings.md) |
| [TransientShaper](TransientShaper.md) | Signed Attack/Sustain masks and permanent limiting | [Findings](TransientShaperFindings.md) |
| [Air](Air.md) | High-frequency contour and cubic harmonic enhancement | [Findings](AirFindings.md) |
| [Thick](Thick.md) | Bass body and finite dense harmonic enhancement | [Findings](ThickFindings.md) |

Each model has one algorithm document `<Processor>.md` and a separate `<Processor>Findings.md` for research observations.
The header owns controls, immutable constants and runtime state. The implementation
stores each fixed `*Config` as an `inline static constexpr kConfig`: configuration
is compiled into the algorithm and shared by instances. It has no runtime setters
or EmulationLab overrides. Per-instance storage contains live controls, prepared
sample-rate coefficients and signal history.

The implementation
separates preparation, detector updates, gain calculation and audio processing
into named private methods. Call order preserves the existing sample history,
channel linking, parameter ramps and clamp/filter placement.

Native and [VST3 adapters](../../../EmulationLab/Docs/Aspects/ModelPluginsVst3.md) compile the same
sources. Adapters remain in ModelPlugins.Vst3 and contain only host controls/state,
bypass and preallocated format conversion. No duplicate algorithms, namespace
aliases or compatibility headers are provided at the former locations.

Shared primitives and runtime composites remain in the parent Dsp folder.
Compressor owns TransientShaper; Polish composes Thick then Air. CLA models remain
standalone Native processors compiled by their VST3 adapters, without graph/ABI
integration. [DSP rules](../Rules.md) still apply.
