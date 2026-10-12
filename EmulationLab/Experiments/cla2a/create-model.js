const fs=require('fs');const src='Consolidator.Native/External/Dsp/Processors/Cla3a/',dst='Consolidator.Native/External/Dsp/Processors/Cla2a/';fs.mkdirSync(dst,{recursive:true});
let h=fs.readFileSync(src+'Cla3aProcessor.h','utf8').replaceAll('Cla3a','Cla2a');
h=h.replace('// Amplitude controls: Input scales only detector sensitivity; Output scales audio.','// Normalized reference knobs; all other instantiated reference settings stay fixed.').replace('double inputGain{ 0.03 };','double gain{ 0.535 };').replace('double outputGain{ 0.12 };','double peakReduction{ 0.598 };');
h=h.replace('    double inputCalibrationDb{ 16.0 };\n    double outputCalibrationDb{ 30.0 };\n    double thresholdDb{ -25.7 };',`    double gainUnityPosition{ 0.408 };
    double gainSpanDb{ 50.0 };
    // Seed curve in u=(Peak Reduction-0.6)/0.4; replaced by measured identification.
    std::array<double, 4> thresholdLaw{ -27.37, -22.94, 14.42, -5.13 };`);
h=h.replace('double fastAttackSeconds{ 0.005 }','double fastAttackSeconds{ 0.010 }').replace('double slowAttackSeconds{ 0.075 }','double slowAttackSeconds{ 0.080 }').replace('double fastReleaseSeconds{ 0.105 }','double fastReleaseSeconds{ 0.065 }').replace('double slowReleaseSeconds{ 0.8 }','double slowReleaseSeconds{ 1.5 }').replace('double slowFraction{ 0.18 }','double slowFraction{ 0.3 }').replace('double highShelfDb{ 2.2 }','double highShelfDb{ 0.0 }');
h=h.replace('    double detectorShelfHz{ 740 };\n    double detectorLowDb{ -1.8 };\n','');
h=h.replace('void SetControls(double inputGain, double outputGain)','void SetControls(double gain, double peakReduction)').replace('std::size_t index, double inputGain)','std::size_t index)');
h=h.replace('    DspParameterRamp input_{ Cla2aControls{}.inputGain };\n    DspParameterRamp output_{ Cla2aControls{}.outputGain };',`    DspParameterRamp gain_{ Cla2aControls{}.gain };
    DspParameterRamp peakReduction_{ Cla2aControls{}.peakReduction };
    double cachedGain_{ -1.0 };
    double cachedPeakReduction_{ -1.0 };
    double thresholdDb_{};
    double outputGain_{};`);
h=h.replace('    double detectorShelfCoefficient_{};\n    double detectorLowLoss_{};\n    std::array<std::array<double, 1>, 2> detectorShelfStates_{};\n','').replace('    double inputCalibration_{};\n    double outputCalibration_{};\n','');
h=h.replace('    void PrepareDetector(double sampleRate) noexcept;', '    void PrepareDetector(double sampleRate) noexcept;\n    void UpdateReductionCurve(double peakReduction) noexcept;');
fs.writeFileSync(dst+'Cla2aProcessor.h',h);
let c=fs.readFileSync(src+'Cla3aProcessor.cpp','utf8').replaceAll('Cla3a','Cla2a');
c=c.replace('#include "../../DspOnePole.h"\n','');
c=c.replace('    input_.Prepare(context);\n    output_.Prepare(context);\n    inputCalibration_ = DbToGain(kConfig.inputCalibrationDb);\n    outputCalibration_ = DbToGain(kConfig.outputCalibrationDb);','    gain_.Prepare(context);\n    peakReduction_.Prepare(context);');
c=c.replace('    input_.Reset();\n    output_.Reset();','    gain_.Reset();\n    peakReduction_.Reset();\n    cachedGain_ = -1.0;\n    cachedPeakReduction_ = -1.0;\n    UpdateReductionCurve(peakReduction_.CurrentValue());');
c=c.replace('    detectorShelfStates_ = {};\n','');
c=c.replace('void Cla2aProcessor::SetControls(double inputGain, double outputGain) noexcept\n{\n    input_.SetTarget(inputGain);\n    output_.SetTarget(outputGain);\n}',`void Cla2aProcessor::SetControls(double gain, double peakReduction) noexcept
{
    gain_.SetTarget(gain);
    peakReduction_.SetTarget(peakReduction);
}`);
c=c.replace('        const auto inputGain = input_.NextValue() * inputCalibration_;','        const auto peakReduction = peakReduction_.NextValue();\n        UpdateReductionCurve(peakReduction);');
c=c.replace('        const auto targetDb = CalculateTargetReductionDb();','        const auto targetDb = peakReduction > 0.0 ? CalculateTargetReductionDb() : 0.0;').replace('UpdateDetector(channels, index, inputGain);','UpdateDetector(channels, index);');
c=c.replace('        selected.ApplyGain(index, output_.NextValue() * outputCalibration_);',`        const auto gain = gain_.NextValue();
        if (gain != cachedGain_)
        {
            outputGain_ = DbToGain((gain - kConfig.gainUnityPosition) * kConfig.gainSpanDb);
            cachedGain_ = gain;
        }
        selected.ApplyGain(index, outputGain_);`);
c=c.replace('    const auto start = DbToGain(kConfig.thresholdDb - kConfig.kneeDb * 0.5);\n    compressionStartPower_ = start * start;\n','');
c=c.replace('    detectorShelfCoefficient_ = onepole::FilterCoefficient(kConfig.detectorShelfHz, sampleRate);\n    detectorLowLoss_ = 1.0 - DbToGain(kConfig.detectorLowDb);\n','');
c=c.replace(' - kConfig.thresholdDb;', ' - thresholdDb_;');
c=c.replace('    const std::array<double*, 2>& channels, std::size_t index, double inputGain) noexcept','    const std::array<double*, 2>& channels, std::size_t index) noexcept');
c=c.replace(`            const auto high = onepole::HighPass(
                sample, detectorShelfCoefficient_, detectorShelfStates_[channel]);
            const auto detectorSample = sample - detectorLowLoss_ * (sample - high);
            amplitude = std::max(amplitude, std::abs(detectorSample));`,'            amplitude = std::max(amplitude, std::abs(sample));');
c=c.replace('    const auto detectorAmplitude = amplitude * inputGain;\n    const auto power = detectorAmplitude * detectorAmplitude;','    const auto power = amplitude * amplitude;');
const update=`void Cla2aProcessor::UpdateReductionCurve(double peakReduction) noexcept
{
    if (peakReduction == cachedPeakReduction_)
    {
        return;
    }
    const auto position = (peakReduction - 0.6) / 0.4;
    const auto& c = kConfig.thresholdLaw;
    thresholdDb_ = c[0] + position * (c[1] + position * (c[2] + position * c[3]));
    const auto start = DbToGain(thresholdDb_ - kConfig.kneeDb * 0.5);
    compressionStartPower_ = start * start;
    cachedPeakReduction_ = peakReduction;
}

`;
c=c.replace('void Cla2aProcessor::PrepareColourFilters',update+'void Cla2aProcessor::PrepareColourFilters');
fs.writeFileSync(dst+'Cla2aProcessor.cpp',c);
let w=fs.readFileSync('ModelPlugins.Vst3/Source/Cla3aPluginProcessor.cpp','utf8').replaceAll('Cla3a','Cla2a').replaceAll('CLA-3A','CLA-2A');
let a=w.indexOf('void AddAttenuation('),b=w.indexOf('juce::AudioProcessorValueTreeState::ParameterLayout',a);
w=w.slice(0,a)+`void AddAmount(
    std::vector<std::unique_ptr<juce::RangedAudioParameter>>& parameters,
    const char* id, const char* name, double defaultValue)
{
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(id, 1), name, juce::NormalisableRange<float>(0.0F, 1.0F),
        static_cast<float>(defaultValue)));
}

`+w.slice(b);
w=w.replace('AddAttenuation(values, "input", "Input", defaults.inputGain);','AddAmount(values, "gain", "Gain", defaults.gain);').replace('AddAttenuation(values, "output", "Output", defaults.outputGain);','AddAmount(values, "peakReduction", "Peak Reduction", defaults.peakReduction);');
w=w.replaceAll('input_', 'gain_').replaceAll('output_', 'peakReduction_').replace('getRawParameterValue("input")','getRawParameterValue("gain")').replace('getRawParameterValue("output")','getRawParameterValue("peakReduction")');
fs.writeFileSync('ModelPlugins.Vst3/Source/Cla2aPluginProcessor.cpp',w);
