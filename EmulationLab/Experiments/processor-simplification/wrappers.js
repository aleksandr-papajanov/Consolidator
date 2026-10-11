const fs=require('fs');
let f='ModelPlugins.Vst3/Source/ThickAirModelPluginProcessor.cpp',t=fs.readFileSync(f,'utf8');
let a=t.indexOf('#if defined(CONSOLIDATOR_THICK_MODEL)',t.indexOf('juce::AudioProcessorValueTreeState::ParameterLayout CreateParameterLayout')),b=t.indexOf('    return juce::AudioProcessorValueTreeState::ParameterLayout',a);
t=t.slice(0,a)+`    std::vector<std::unique_ptr<juce::RangedAudioParameter>> values;
#if defined(CONSOLIDATOR_THICK_MODEL)
    AddFloat(values, "thick", "Thick", 0.0, 1.0, 0.0);
#else
    AddFloat(values, "air", "Air", 0.0, 1.0, 0.0);
#endif
    values.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));
`+t.slice(b);
t=t.replace('        processor_.SetTuningBeforePrepare(ReadTuning());\n','');
a=t.indexOf('    double Value(');b=t.indexOf('#if defined(CONSOLIDATOR_THICK_MODEL)\n    PolishThickProcessor processor_;',a);t=t.slice(0,a)+t.slice(b);
t=t.replace('#include <string>\n','#include <cmath>\n');fs.writeFileSync(f,t);
f='ModelPlugins.Vst3/Source/TransientShaperPluginProcessor.cpp';t=fs.readFileSync(f,'utf8');a=t.indexOf('    max::dsp::TransientShaperTuning tuning;');b=t.indexOf('    return juce::AudioProcessorValueTreeState::ParameterLayout',a);
t=t.slice(0,a)+'    AddFloat(values, "outputGainDb", "Output Gain dB", -12, 12, 0);\n'+t.slice(b);
t=t.replace('        transientShaper_.SetTuningBeforePrepare(ReadMaskTuning());\n','');
a=t.indexOf('    double Value(');b=t.indexOf('    max::dsp::TransientShaperProcessor transientShaper_;',a);t=t.slice(0,a)+t.slice(b);t=t.replace('#include <cstddef>','#include <cstddef>\n#include <cmath>');fs.writeFileSync(f,t);
const state=`            const auto saved = juce::ValueTree::fromXml(*xml);
            auto state = parameters_.copyState();
            for (const auto child : state)
            {
                const auto id = child.getProperty("id").toString();
                const auto control = saved.getChildWithProperty("id", id);
                if (!control.isValid() || !control.hasProperty("value") ||
                    !std::isfinite(static_cast<float>(control.getProperty("value"))))
                {
                    juce::Logger::writeToLog(getName() + " state is missing a valid user control: " + id);
                    return;
                }
            }
            for (auto child : state)
            {
                const auto id = child.getProperty("id").toString();
                child.setProperty("value", saved.getChildWithProperty("id", id).getProperty("value"), nullptr);
            }
            parameters_.replaceState(state);`;
for(const file of ['ModelPlugins.Vst3/Source/TransientShaperPluginProcessor.cpp','ModelPlugins.Vst3/Source/ThickAirModelPluginProcessor.cpp']){t=fs.readFileSync(file,'utf8');t=t.replace('            parameters_.replaceState(juce::ValueTree::fromXml(*xml));',state);fs.writeFileSync(file,t);}
f='Consolidator.Native/External/Dsp/PolishProcessor.h';t=fs.readFileSync(f,'utf8');t=t.replace(/    void SetTuningsBeforePrepare\(\s*const PolishThickTuning& thick,\s*const PolishAirTuning& air\) noexcept;\n/,'');fs.writeFileSync(f,t);
f='Consolidator.Native/External/Dsp/PolishProcessor.cpp';t=fs.readFileSync(f,'utf8');a=t.indexOf('void PolishProcessor::SetTuningsBeforePrepare(');b=t.indexOf('void PolishProcessor::Prepare',a);t=t.slice(0,a)+t.slice(b);fs.writeFileSync(f,t);
