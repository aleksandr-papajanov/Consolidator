# Finished DSP processors

These five models are complete. Production code and its algorithm documentation
live together in one folder per processor. C++ namespace:
`consolidator::max::dsp::processors`.

| Folder | Algorithm | Research notes |
| --- | --- | --- |
| [Cla76](Cla76/README.md) | FET compressor, continuous 4:1 to ALL | [Findings](Cla76/Findings.md) |
| [Cla3a](Cla3a/README.md) | Optical compressor, detector Input and audio Output | [Findings](Cla3a/Findings.md) |
| [TransientShaper](TransientShaper/README.md) | Signed Attack/Sustain masks and permanent limiting | [Findings](TransientShaper/Findings.md) |
| [Air](Air/README.md) | High-frequency contour and cubic harmonic enhancement | [Findings](Air/Findings.md) |
| [Thick](Thick/README.md) | Bass body and finite dense harmonic enhancement | [Findings](Thick/Findings.md) |

Each folder contains the processor's `.h/.cpp`, `README.md` and `Findings.md`.
The header owns controls, immutable constants and runtime state. The implementation
stores each fixed `*Config` as an `inline static constexpr kConfig`: configuration
is compiled into the algorithm and shared by instances. It has no runtime setters
or EmulationLab overrides. Per-instance storage contains live controls, prepared
sample-rate coefficients and signal history.

The implementation
separates preparation, detector updates, gain calculation and audio processing
into named private methods. Call order preserves the existing sample history,
channel linking, parameter ramps and clamp/filter placement.

Native and [VST3 adapters](../../../../Docs/Dsp/ModelPluginsVst3.md) compile the same
sources. Adapters remain in ModelPlugins.Vst3 and contain only host controls/state,
bypass and preallocated format conversion. No duplicate algorithms, namespace
aliases or compatibility headers are provided at the former locations.

Shared primitives and runtime composites remain in the parent Dsp folder.
Compressor owns TransientShaper; Polish composes Thick then Air. CLA models remain
standalone Native processors compiled by their VST3 adapters, without graph/ABI
integration. [DSP rules](../../../../Docs/Dsp/Rules.md) still apply.
