#include "Diagnostics.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <fstream>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "Host/Vst3Host.h"

namespace emulation_lab
{

namespace
{

constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 128;
constexpr double kFrequency = 1000.0;
constexpr double kWindowSeconds = 0.020;
constexpr double kOverlayWindowSeconds = 0.002;
constexpr double kComponentWindowSeconds = 0.010;
constexpr double kMinimumLevel = 1.0e-12;

std::size_t Samples(double seconds)
{
    return static_cast<std::size_t>(std::round(seconds * kSampleRate));
}

double Db(double amplitude)
{
    return 20.0 * std::log10(std::max(amplitude, kMinimumLevel));
}

void FillTone(
    std::vector<double>& input,
    std::size_t start,
    std::size_t end,
    double levelDb)
{
    const auto amplitude = std::pow(10.0, levelDb / 20.0);
    for (auto index = start; index < end; ++index)
    {
        input[index] = amplitude * std::sin(
            2.0 * std::numbers::pi * kFrequency *
            static_cast<double>(index) / kSampleRate);
    }
}

void AddWindowedAttack(
    std::vector<double>& input,
    double onsetSeconds,
    double frequencyHz,
    double levelDb)
{
    const auto onset = Samples(onsetSeconds);
    const auto duration = Samples(0.010);
    const auto amplitude = std::pow(10.0, levelDb / 20.0);
    for (std::size_t index = onset; index < onset + duration; ++index)
    {
        const auto position = static_cast<double>(index - onset) / duration;
        const auto window = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * position);
        input[index] += amplitude * window * std::sin(
            2.0 * std::numbers::pi * frequencyHz * index / kSampleRate);
    }
}

nlohmann::json AnalyzeWindows(
    std::span<const double> input,
    std::span<const double> output,
    std::size_t latency)
{
    nlohmann::json windows = nlohmann::json::array();
    const auto count = Samples(kWindowSeconds);
    for (std::size_t start = 0; start + count <= input.size(); start += count)
    {
        auto inputEnergy = 0.0;
        auto outputEnergy = 0.0;
        auto inputPeak = 0.0;
        auto outputPeak = 0.0;
        for (std::size_t index = start; index < start + count; ++index)
        {
            const auto in = input[index];
            const auto out = output[index + latency];
            inputEnergy += in * in;
            outputEnergy += out * out;
            inputPeak = std::max(inputPeak, std::abs(in));
            outputPeak = std::max(outputPeak, std::abs(out));
        }
        const auto inputRms = std::sqrt(inputEnergy / static_cast<double>(count));
        const auto outputRms = std::sqrt(outputEnergy / static_cast<double>(count));
        windows.push_back({
            { "timeSeconds", (static_cast<double>(start) + count * 0.5) / kSampleRate },
            { "inputDbfs", Db(inputRms) },
            { "outputDbfs", Db(outputRms) },
            { "gainDb", inputRms > kMinimumLevel
                ? nlohmann::json(Db(outputRms / inputRms))
                : nlohmann::json(nullptr) },
            { "inputPeakDbfs", Db(inputPeak) },
            { "outputPeakDbfs", Db(outputPeak) }
        });
    }
    return windows;
}

nlohmann::json Render(
    Vst3Plugin& plugin,
    const std::string& name,
    std::vector<double> input)
{
    plugin.Reprepare();
    const auto latency = plugin.LatencySamples();
    const auto inputSize = input.size();
    input.resize(inputSize + latency, 0.0);
    const auto output = plugin.Process(input);
    if (output.size() != input.size())
    {
        throw std::runtime_error("Diagnostic render returned an incomplete buffer.");
    }
    return {
        { "name", name },
        { "latencySamples", latency },
        { "windows", AnalyzeWindows(
            std::span<const double>(input.data(), inputSize), output, latency) }
    };
}

double ToneMagnitude(
    std::span<const double> samples,
    double frequency)
{
    std::complex<double> projection{};
    for (std::size_t index = 0; index < samples.size(); ++index)
    {
        const auto phase = -2.0 * std::numbers::pi * frequency *
            static_cast<double>(index) / kSampleRate;
        projection += samples[index] * std::polar(1.0, phase);
    }
    return 2.0 * std::abs(projection) / static_cast<double>(samples.size());
}

nlohmann::json RenderTwoTone(Vst3Plugin& plugin)
{
    constexpr double firstHz = 997.0;
    constexpr double secondHz = 1201.0;
    plugin.Reprepare();
    const auto latency = plugin.LatencySamples();
    std::vector<double> input(Samples(2.0) + latency);
    const auto amplitude = std::pow(10.0, -18.0 / 20.0) * 0.5;
    for (std::size_t index = 0; index < input.size(); ++index)
    {
        const auto seconds = static_cast<double>(index) / kSampleRate;
        input[index] = amplitude * (
            std::sin(2.0 * std::numbers::pi * firstHz * seconds) +
            std::sin(2.0 * std::numbers::pi * secondHz * seconds));
    }
    const auto output = plugin.Process(input);
    if (output.size() != input.size())
    {
        throw std::runtime_error("Diagnostic two-tone render returned an incomplete buffer.");
    }
    const auto start = Samples(1.0) + latency;
    const auto measured = std::span<const double>(output.data() + start, Samples(1.0));
    const auto primary = std::max(
        ToneMagnitude(measured, firstHz),
        ToneMagnitude(measured, secondHz));
    nlohmann::json products = nlohmann::json::array();
    for (const auto frequency : { 204.0, 2198.0, 793.0, 1405.0 })
    {
        products.push_back({
            { "frequencyHz", frequency },
            { "levelDbc", Db(ToneMagnitude(measured, frequency) /
                std::max(primary, kMinimumLevel)) }
        });
    }
    return {
        { "name", "two-tone-997-1201" },
        { "primaryDbfs", Db(primary) },
        { "products", std::move(products) }
    };
}

nlohmann::json AnalyzeOverlayWindows(
    std::span<const double> background,
    std::span<const double> overlay,
    std::span<const double> backgroundOutput,
    std::span<const double> overlayOutput,
    std::size_t latency)
{
    nlohmann::json windows = nlohmann::json::array();
    const auto count = Samples(kOverlayWindowSeconds);
    for (auto start = Samples(0.96); start + count <= Samples(1.16); start += count)
    {
        auto inputEnergy = 0.0;
        auto outputEnergy = 0.0;
        auto backgroundEnergy = 0.0;
        for (auto index = start; index < start + count; ++index)
        {
            const auto inputDifference = overlay[index] - background[index];
            const auto outputDifference =
                overlayOutput[index + latency] - backgroundOutput[index + latency];
            inputEnergy += inputDifference * inputDifference;
            outputEnergy += outputDifference * outputDifference;
            backgroundEnergy += backgroundOutput[index + latency] *
                backgroundOutput[index + latency];
        }
        const auto inputRms = std::sqrt(inputEnergy / static_cast<double>(count));
        const auto outputRms = std::sqrt(outputEnergy / static_cast<double>(count));
        windows.push_back({
            { "timeSeconds", (static_cast<double>(start) + count * 0.5) / kSampleRate },
            { "backgroundOutputDbfs", Db(std::sqrt(backgroundEnergy / count)) },
            { "inputDifferenceDbfs", Db(inputRms) },
            { "outputDifferenceDbfs", Db(outputRms) },
            { "differenceGainDb", inputRms > kMinimumLevel
                ? nlohmann::json(Db(outputRms / inputRms))
                : nlohmann::json(nullptr) }
        });
    }
    return windows;
}

nlohmann::json AnalyzeOverlayComponents(
    std::span<const double> backgroundOutput,
    std::span<const double> overlayOutput,
    std::size_t latency,
    double attackHz,
    double onsetSeconds)
{
    nlohmann::json windows = nlohmann::json::array();
    const auto count = Samples(kComponentWindowSeconds);
    for (auto start = Samples(onsetSeconds - 0.01);
        start + count <= Samples(onsetSeconds + 0.80); start += count)
    {
        auto sine200 = 0.0;
        auto cosine200 = 0.0;
        auto baselineSine200 = 0.0;
        auto baselineCosine200 = 0.0;
        auto sine1000 = 0.0;
        auto cosine1000 = 0.0;
        auto totalEnergy = 0.0;
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto sample = overlayOutput[start + index + latency] -
                backgroundOutput[start + index + latency];
            const auto baseline = backgroundOutput[start + index + latency];
            const auto phase200 = 2.0 * std::numbers::pi * 200.0 * index / kSampleRate;
            const auto phase1000 = 2.0 * std::numbers::pi * attackHz * index / kSampleRate;
            sine200 += sample * std::sin(phase200);
            cosine200 += sample * std::cos(phase200);
            baselineSine200 += baseline * std::sin(phase200);
            baselineCosine200 += baseline * std::cos(phase200);
            sine1000 += sample * std::sin(phase1000);
            cosine1000 += sample * std::cos(phase1000);
            totalEnergy += sample * sample;
        }
        const auto scale = 2.0 / static_cast<double>(count);
        sine200 *= scale;
        cosine200 *= scale;
        baselineSine200 *= scale;
        baselineCosine200 *= scale;
        sine1000 *= scale;
        cosine1000 *= scale;
        const auto backgroundEnergy = 0.5 * (
            sine200 * sine200 + cosine200 * cosine200);
        const auto attackEnergy = 0.5 * (
            sine1000 * sine1000 + cosine1000 * cosine1000);
        const auto totalMeanSquare = totalEnergy / static_cast<double>(count);
        const auto baselinePower = baselineSine200 * baselineSine200 +
            baselineCosine200 * baselineCosine200;
        const auto relativeReal = baselinePower > kMinimumLevel * kMinimumLevel
            ? (sine200 * baselineSine200 + cosine200 * baselineCosine200) /
                baselinePower
            : 0.0;
        const auto relativeImaginary = baselinePower > kMinimumLevel * kMinimumLevel
            ? (cosine200 * baselineSine200 - sine200 * baselineCosine200) /
                baselinePower
            : 0.0;
        const auto otherEnergy = std::max(
            0.0, totalMeanSquare - backgroundEnergy - attackEnergy);
        windows.push_back({
            { "timeSeconds", (static_cast<double>(start) + count * 0.5) / kSampleRate },
            { "background200DifferenceDbfs", Db(std::sqrt(backgroundEnergy)) },
            { "attackCarrierDifferenceDbfs", Db(std::sqrt(attackEnergy)) },
            { "otherDifferenceDbfs", Db(std::sqrt(otherEnergy)) },
            { "totalDifferenceDbfs", Db(std::sqrt(totalMeanSquare)) },
            { "background200EnergyFraction", totalMeanSquare > kMinimumLevel * kMinimumLevel
                ? nlohmann::json(backgroundEnergy / totalMeanSquare)
                : nlohmann::json(nullptr) },
            { "background200GainChangeDb", baselinePower > kMinimumLevel * kMinimumLevel
                ? nlohmann::json(Db(std::hypot(1.0 + relativeReal, relativeImaginary)))
                : nlohmann::json(nullptr) },
            { "background200PhaseChangeDegrees", baselinePower > kMinimumLevel * kMinimumLevel
                ? nlohmann::json(std::atan2(relativeImaginary, 1.0 + relativeReal) *
                    180.0 / std::numbers::pi)
                : nlohmann::json(nullptr) }
        });
    }
    return windows;
}

nlohmann::json RenderOverlay(
    Vst3Plugin& plugin,
    double backgroundDb,
    double attackHz,
    double attackDb)
{
    std::vector<double> background(Samples(2.0));
    std::vector<double> overlay(background.size());
    const auto backgroundAmplitude = std::pow(10.0, backgroundDb / 20.0);
    for (std::size_t index = 0; index < background.size(); ++index)
    {
        const auto base = backgroundAmplitude * std::sin(
            2.0 * std::numbers::pi * 200.0 * index / kSampleRate);
        background[index] = base;
        overlay[index] = base;
    }
    AddWindowedAttack(overlay, 1.0, attackHz, attackDb);
    plugin.Reprepare();
    const auto latency = plugin.LatencySamples();
    auto backgroundRender = background;
    backgroundRender.resize(background.size() + latency, 0.0);
    const auto backgroundOutput = plugin.Process(backgroundRender);
    plugin.Reprepare();
    auto overlayRender = overlay;
    overlayRender.resize(overlay.size() + latency, 0.0);
    const auto overlayOutput = plugin.Process(overlayRender);
    if (backgroundOutput.size() != backgroundRender.size() ||
        overlayOutput.size() != overlayRender.size())
    {
        throw std::runtime_error("Diagnostic overlay render returned an incomplete buffer.");
    }
    return {
        { "name", "attack-on-background-" + std::to_string(static_cast<int>(backgroundDb)) +
            "-" + std::to_string(static_cast<int>(attackHz)) + "hz-" +
            std::to_string(static_cast<int>(attackDb)) + "db" },
        { "backgroundDbfs", backgroundDb },
        { "attackDbfs", attackDb },
        { "attackHz", attackHz },
        { "attackDurationMs", 10.0 },
        { "latencySamples", latency },
        { "windows", AnalyzeOverlayWindows(
            background, overlay, backgroundOutput, overlayOutput, latency) },
        { "components", AnalyzeOverlayComponents(
            backgroundOutput, overlayOutput, latency, attackHz, 1.0) }
    };
}

nlohmann::json RenderDoublePulse(
    Vst3Plugin& plugin,
    double backgroundDb,
    double gapSeconds)
{
    std::vector<double> background(Samples(3.0));
    const auto amplitude = std::pow(10.0, backgroundDb / 20.0);
    for (std::size_t index = 0; index < background.size(); ++index)
    {
        background[index] = amplitude * std::sin(
            2.0 * std::numbers::pi * 200.0 * index / kSampleRate);
    }
    auto first = background;
    auto second = background;
    auto paired = background;
    AddWindowedAttack(first, 1.0, kFrequency, -12.0);
    AddWindowedAttack(paired, 1.0, kFrequency, -12.0);
    const auto secondOnset = 1.0 + gapSeconds;
    AddWindowedAttack(second, secondOnset, kFrequency, -12.0);
    AddWindowedAttack(paired, secondOnset, kFrequency, -12.0);

    plugin.Reprepare();
    const auto latency = plugin.LatencySamples();
    const auto render = [&plugin, latency](const std::vector<double>& input)
    {
        plugin.Reprepare();
        auto padded = input;
        padded.resize(input.size() + latency, 0.0);
        auto output = plugin.Process(padded);
        if (output.size() != padded.size())
        {
            throw std::runtime_error("Diagnostic double-pulse render returned an incomplete buffer.");
        }
        return output;
    };
    const auto backgroundOutput = render(background);
    const auto firstOutput = render(first);
    const auto secondOutput = render(second);
    const auto pairedOutput = render(paired);
    return {
        { "name", "double-pulse-background-" + std::to_string(static_cast<int>(backgroundDb)) +
            "-gap-" + std::to_string(std::lround(gapSeconds * 1000.0)) + "ms" },
        { "backgroundDbfs", backgroundDb },
        { "gapMs", gapSeconds * 1000.0 },
        { "attackDbfs", -12.0 },
        { "attackHz", kFrequency },
        { "secondOnsetSeconds", secondOnset },
        { "latencySamples", latency },
        { "isolatedSecond", AnalyzeOverlayComponents(
            backgroundOutput, secondOutput, latency, kFrequency, secondOnset) },
        { "afterFirst", AnalyzeOverlayComponents(
            firstOutput, pairedOutput, latency, kFrequency, secondOnset) }
    };
}

} // namespace

void RunDiagnostics(
    const ExperimentManifest& manifest,
    const std::string& side,
    const std::string& conditionId,
    const std::filesystem::path& outputPath,
    const std::filesystem::path& fitPath)
{
    if (side != "reference" && side != "candidate")
    {
        throw std::runtime_error("Diagnostic side must be reference or candidate.");
    }
    const auto condition = std::find_if(
        manifest.conditions.begin(), manifest.conditions.end(),
        [&conditionId](const auto& value) { return value.identifier == conditionId; });
    if (condition == manifest.conditions.end())
    {
        throw std::runtime_error("Diagnostic condition was not found: " + conditionId);
    }

    const auto reference = side == "reference";
    const auto& pluginPath = reference ? manifest.referencePlugin : manifest.candidatePlugin;
    const auto& query = reference ? manifest.referenceQuery : manifest.candidateQuery;
    const auto& state = reference ? condition->referenceState : condition->candidateState;
    const auto& baseState = reference ? manifest.referenceState : manifest.candidateState;
    const auto& controls = reference ? condition->referenceControls : condition->candidateControls;
    Vst3Host host;
    std::string error;
    auto plugin = host.Open(pluginPath, query, kSampleRate, kBlockSize, error);
    if (plugin == nullptr)
    {
        throw std::runtime_error(error);
    }
    const auto& effectiveState = state.empty() ? baseState : state;
    if (!effectiveState.empty() && !plugin->LoadState(effectiveState))
    {
        throw std::runtime_error("Could not load diagnostic plugin state: " + effectiveState);
    }
    nlohmann::json appliedControls = nlohmann::json::array();
    for (const auto& control : controls)
    {
        const auto applied = control.usesNormalizedValue
            ? plugin->SetParameterNormalized(control.name, control.normalizedValue)
            : plugin->SetParameterText(control.name, control.text);
        PluginParameterDescription readback;
        if (!applied || !plugin->Parameter(control.name, readback))
        {
            throw std::runtime_error("Diagnostic control could not be applied: " + control.name);
        }
        appliedControls.push_back({
            { "name", readback.name },
            { "normalized", readback.normalizedValue },
            { "text", readback.text }
        });
    }
    if (!fitPath.empty())
    {
        if (reference)
        {
            throw std::runtime_error("A fit can only be applied to a candidate diagnostic.");
        }
        std::ifstream input(fitPath);
        if (!input)
        {
            throw std::runtime_error("Could not open diagnostic fit: " + fitPath.string());
        }
        nlohmann::json fit;
        input >> fit;
        if (fit.value("experiment", "") != manifest.name ||
            fit.value("configurationSignature", "") !=
                MeasurementConfigurationSignature(manifest))
        {
            throw std::runtime_error("Diagnostic fit does not match the experiment.");
        }
        for (const auto& value : fit.at("parameters"))
        {
            const auto name = value.at("pluginParameter").get<std::string>();
            const auto normalized = value.at("value").get<double>();
            if (!std::isfinite(normalized) || normalized < 0.0 ||
                normalized > 1.0 || !plugin->SetParameterNormalized(name, normalized))
            {
                throw std::runtime_error("Invalid diagnostic fit parameter: " + name);
            }
        }
    }
    nlohmann::json scenarios = nlohmann::json::array();
    std::vector<double> isolated(Samples(5.0));
    FillTone(isolated, 0, isolated.size(), -18.0);
    scenarios.push_back(Render(*plugin, "isolated-5s-minus18", std::move(isolated)));
    for (const auto levelDb : { -6.0, 0.0 })
    {
        std::vector<double> raised(Samples(5.0));
        FillTone(raised, 0, raised.size(), levelDb);
        scenarios.push_back(Render(
            *plugin, levelDb == 0.0 ? "isolated-5s-zero" : "isolated-5s-minus6",
            std::move(raised)));
    }

    std::vector<double> step(Samples(6.0));
    FillTone(step, 0, Samples(2.0), -30.0);
    FillTone(step, Samples(2.0), Samples(4.0), -6.0);
    FillTone(step, Samples(4.0), step.size(), -30.0);
    scenarios.push_back(Render(*plugin, "level-step-minus30-minus6-minus30", std::move(step)));

    for (const auto burstMs : { 2.0, 10.0, 50.0, 500.0 })
    {
        std::vector<double> burst(Samples(1.0 + burstMs / 1000.0));
        FillTone(burst, Samples(0.25), Samples(0.25 + burstMs / 1000.0), -18.0);
        scenarios.push_back(Render(
            *plugin, "burst-" + std::to_string(static_cast<int>(burstMs)) + "ms",
            std::move(burst)));
    }
    for (const auto backgroundDb : { -30.0, -18.0, -9.0 })
    {
        for (const auto attackDb : { -24.0, -12.0, -6.0 })
        {
            scenarios.push_back(RenderOverlay(*plugin, backgroundDb, 1000.0, attackDb));
        }
        scenarios.push_back(RenderOverlay(*plugin, backgroundDb, 5000.0, -12.0));
    }
    for (const auto backgroundDb : { -30.0, -9.0 })
    {
        for (const auto gapSeconds : {
            0.025, 0.050, 0.100, 0.150, 0.200, 0.400, 0.800 })
        {
            scenarios.push_back(RenderDoublePulse(*plugin, backgroundDb, gapSeconds));
        }
    }
    scenarios.push_back(RenderTwoTone(*plugin));

    const nlohmann::json report{
        { "schemaVersion", 4 },
        { "experiment", manifest.name },
        { "side", side },
        { "condition", conditionId },
        { "fitPath", fitPath.string() },
        { "pluginIdentity", plugin->Identity() },
        { "sampleRate", kSampleRate },
        { "windowSeconds", kWindowSeconds },
        { "frequencyHz", kFrequency },
        { "appliedControls", std::move(appliedControls) },
        { "scenarios", std::move(scenarios) }
    };
    const auto parent = outputPath.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }
    std::ofstream output(outputPath);
    if (!output)
    {
        throw std::runtime_error("Could not write diagnostic report.");
    }
    output << report.dump(2) << '\n';
}

} // namespace emulation_lab
