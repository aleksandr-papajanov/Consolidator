const fs=require('fs'),root='Consolidator.Native/External/Dsp/';
let t=fs.readFileSync(root+'PolishThickProcessor.cpp','utf8');
const begin=t.indexOf('constexpr std::size_t kDenseMaximumHarmonic'),end=t.indexOf('double GenerateBass(',begin);
t=t.slice(0,begin)+`double BodyMixForLevel(double amplitude, const PolishThickTuning& tuning) noexcept
{
    const auto x = std::sqrt(std::min(amplitude, 1.0));
    const auto& c = tuning.bodyMixLaw;
    return std::clamp(tuning.bodyMixFloor +
        (1.0 - x) * (c[0] + x * (c[1] + x * c[2])) / (1.0 + c[3] * x),
        tuning.bodyMixFloor, 1.0);
}

double GenerateDenseHarmonics(
    double sample, const PolishThickTuning& tuning, double tailWeight) noexcept
{
    // Fast doubling gives T79/T80 in seven steps. The finite geometric sum
    // preserves every original H2..H159 term, including intentional folding.
    auto current = 1.0;
    auto next = sample;
    constexpr std::size_t pairCount = 79;
    for (std::size_t bit = 64; bit != 0; bit >>= 1)
    {
        const auto even = 2.0 * current * current - 1.0;
        const auto odd = 2.0 * current * next - sample;
        const auto upper = 2.0 * next * next - 1.0;
        if ((pairCount & bit) != 0)
        {
            current = odd;
            next = upper;
        }
        else
        {
            current = even;
            next = odd;
        }
    }
    const auto t158 = 2.0 * current * current - 1.0;
    const auto t159 = 2.0 * current * next - sample;
    const auto t160 = 2.0 * next * next - 1.0;
    const auto t161 = 2.0 * sample * t160 - t159;
    const auto t2 = 2.0 * sample * sample - 1.0;
    const auto t3 = 2.0 * sample * t2 - sample;
    const auto decay = tuning.denseHarmonicDecay;
    const auto denominator = 1.0 - 2.0 * decay * t2 + decay * decay;
    const auto evenSum = t2 - decay - tailWeight * (t160 - decay * t158);
    const auto oddSum = t3 - decay * sample - tailWeight * (t161 - decay * t159);
    return tuning.denseHarmonicGain *
        (oddSum + tuning.evenHarmonicWeight * evenSum) / denominator;
}

`+t.slice(end);
t=t.replace('    const PolishThickTuning& tuning,\n    double& positiveFeedback,','    const PolishThickTuning& tuning,\n    double tailWeight,\n    double& positiveFeedback,');
t=t.replace('GenerateDenseHarmonics(normalized, tuning)','GenerateDenseHarmonics(normalized, tuning, tailWeight)');
let a=t.indexOf('void PolishThickProcessor::SetTuningBeforePrepare('),b=t.indexOf('void PolishThickProcessor::Prepare(',a);t=t.slice(0,a)+t.slice(b);
t=t.replace('    bodyPeak_.Prepare(context);','    bodyPeak_ = {};\n    subNotch_ = {};\n    positiveSubShoulder_ = {};\n    positiveBodyPeak_ = {};\n    bodyPeak_.Prepare(context);');
t=t.replace('    amount_.Prepare(context);','    amount_.Prepare(context);\n    denseTailWeight_ = std::pow(tuning_.denseHarmonicDecay, 79.0);');
t=t.replace('bodyLevelMix_ = tuning_.bodyMixAnchors.front();','bodyLevelMix_ = 1.0;');
t=t.replace(/        const auto bodyControlLevelDb = 20\.0 \* std::log10\(std::max\(\s*bodyControlLevel,\s*1\.0e-8\)\);\s*const auto bodyLevelMixTarget = BodyMixForLevel\(bodyControlLevelDb, tuning_\);/,'        const auto bodyLevelMixTarget = BodyMixForLevel(bodyControlLevel, tuning_);');
t=t.replace(/        const auto positiveLevelMixTarget = std::clamp\(\s*bodyControlLevelDb \/ 10\.0,\s*0\.0,\s*1\.0\);/,'        const auto positiveLevelMixTarget = bodyControlLevel > 1.0\n            ? std::min(2.0 * std::log10(bodyControlLevel), 1.0) : 0.0;');
t=t.replace('                tuning_,\n                state.positiveFeedback,','                tuning_,\n                denseTailWeight_,\n                state.positiveFeedback,');
fs.writeFileSync(root+'PolishThickProcessor.cpp',t);
let p=fs.readFileSync(root+'PolishAirProcessor.cpp','utf8');
a=p.indexOf('namespace\n{',p.indexOf('namespace consolidator'));b=p.indexOf('void PolishAirProcessor::Prepare',a);p=p.slice(0,a)+p.slice(b);
p=p.replace(/    middleShelfCoefficient_ = onepole::FilterCoefficient\(\s*tuning_\.middleShelfFrequencyHz,\s*context.sampleRate\);\s*/,'');
p=p.replace(/    evenHarmonicHighPassCoefficient_ = onepole::FilterCoefficient\(\s*tuning_\.evenHarmonicHighPassHz,\s*context.sampleRate\);\s*/,'');
p=p.replace('    peakHoldRemaining_ = 0;','    peakHoldRemaining_ = 0;\n    linearLevelGain_ = 1.0;');
p=p.replace('    targetAmountGain_ = std::pow(amount, tuning_.amountExponent);\n','');
p=p.replace('        if (peak >= inputPeak_ * 0.999)','        const auto previousPeak = inputPeak_;\n        if (peak >= inputPeak_ * 0.999)');
a=p.indexOf('        const auto levelDb = ');b=p.indexOf('        for (std::size_t channel',a);
p=p.slice(0,a)+`        if (inputPeak_ != previousPeak)
        {
            const auto levelDb = 20.0 * std::log10(std::max(inputPeak_, 1.0e-8));
            const auto position = std::clamp((levelDb + 30.0) / 30.0, 0.0, 1.0);
            const auto& c = tuning_.linearLevelLaw;
            linearLevelGain_ = std::min(1.0, 1.0 + position * (c[0] + c[1] * position));
        }
        const auto amountGain = amount_.NextValue();
        const auto baselineMix = std::min(amountGain * 4.0, 1.0);
        const auto levelLoss = 1.0 - linearLevelGain_;

`+p.slice(b);
p=p.replace(/            const auto middleShelf = onepole::HighPass\(\s*input,\s*middleShelfCoefficient_,\s*state.middleShelf\);\s*/,'');
p=p.replace(/            const auto oddGenerated = cubedSource \* \(\s*1\.0 \+ tuning_\.fifthOrderGain \* squaredSource \+\s*tuning_\.seventhOrderGain \* squaredSource \* squaredSource\);/,'            const auto oddGenerated = cubedSource;');
a=p.indexOf('            const auto evenGenerated');b=p.indexOf('            audio[channel][index]',a);p=p.slice(0,a)+p.slice(b);
p=p.replace('                    tuning_.middleShelfGain * middleShelf +\n','');
p=p.replace('(linearLevelGain - 1.0) *','-levelLoss *').replace('(1.0 - linearLevelGain) *','levelLoss *');
p=p.replace('                tuning_.oddHarmonicGain * oddHarmonics +\n                tuning_.evenHarmonicGain * evenHarmonics);','                tuning_.oddHarmonicGain * oddHarmonics);');
fs.writeFileSync(root+'PolishAirProcessor.cpp',p);
let s=fs.readFileSync(root+'TransientShaperProcessor.cpp','utf8');a=s.indexOf('void TransientShaperProcessor::SetTuningBeforePrepare');b=s.indexOf('void TransientShaperProcessor::Prepare',a);s=s.slice(0,a)+s.slice(b);
s=s.replace('    outputGain_.SetTarget(DbToGain(gainDb));','    if (gainDb != outputGainDb_)\n    {\n        outputGainDb_ = gainDb;\n        outputGain_.SetTarget(DbToGain(gainDb));\n    }');
s=s.replace('1.0 : std::pow(activityRatio, tuning_.activityExponent);','1.0 : std::sqrt(std::sqrt(std::sqrt(std::sqrt(activityRatio))));');
s=s.replace('        const auto combinedGain = std::clamp(\n            baseAndSustainGain * AmountGain(fullAttackGain, attack), 1.0 / maximumGain, maximumGain);\n        const auto attackStageGain = combinedGain / baseAndSustainGain;',`        const auto attackGain = AmountGain(fullAttackGain, attack);
        const auto requestedGain = baseAndSustainGain * attackGain;
        const auto combinedGain = std::clamp(requestedGain, 1.0 / maximumGain, maximumGain);
        const auto attackStageGain = combinedGain == requestedGain
            ? attackGain : combinedGain / baseAndSustainGain;`);
fs.writeFileSync(root+'TransientShaperProcessor.cpp',s);
