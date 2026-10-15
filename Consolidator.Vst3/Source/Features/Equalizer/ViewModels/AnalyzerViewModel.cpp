#include "Features/Equalizer/ViewModels/AnalyzerViewModel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <span>
#include <stdexcept>
#include <utility>

#include "Features/Equalizer/Presentation/AnalyzerScale.h"

namespace consolidator::vst3::features::equalizer::viewmodels
{

namespace
{
constexpr double kQDragSensitivity = 0.02;
constexpr std::array kEqualizerTypes{ features::equalizer::presentation::AnalyzerFilterType::Gain, features::equalizer::presentation::AnalyzerFilterType::Tilt,
    features::equalizer::presentation::AnalyzerFilterType::LowShelf, features::equalizer::presentation::AnalyzerFilterType::HighShelf, features::equalizer::presentation::AnalyzerFilterType::Bell,
    features::equalizer::presentation::AnalyzerFilterType::Bell, features::equalizer::presentation::AnalyzerFilterType::Bell };
constexpr std::array kDetectorTypes{ features::equalizer::presentation::AnalyzerFilterType::LowShelf, features::equalizer::presentation::AnalyzerFilterType::Bell };

template<typename ValueReader>
std::vector<features::equalizer::presentation::AnalyzerFilter> ReadFilters(const std::string& context, ValueReader read)
{
    const bool equalizer = context == "equalizer";
    const std::span<const features::equalizer::presentation::AnalyzerFilterType> types = equalizer
        ? std::span<const features::equalizer::presentation::AnalyzerFilterType>(kEqualizerTypes)
        : std::span<const features::equalizer::presentation::AnalyzerFilterType>(kDetectorTypes);
    const auto prefix = equalizer ? "equalizer.filter." : context + ".detector.filter.";
    std::vector<features::equalizer::presentation::AnalyzerFilter> filters;
    filters.reserve(types.size());
    for (std::size_t index = 0; index < types.size(); ++index)
    {
        const auto path = prefix + std::to_string(index + 1) + ".";
        const auto type = types[index];
        const double gain = shared::protocol::Number(read(path + "gain"));
        const double frequency = type == features::equalizer::presentation::AnalyzerFilterType::Gain ? 0 : shared::protocol::Number(read(path + "frequency"));
        const double q = type == features::equalizer::presentation::AnalyzerFilterType::Bell ? shared::protocol::Number(read(path + "q")) : 0.707;
        if (!std::isfinite(gain) || (type != features::equalizer::presentation::AnalyzerFilterType::Gain &&
            (!std::isfinite(frequency) || frequency <= 0 || !std::isfinite(q) || q <= 0)))
        {
            throw std::invalid_argument("Invalid analyzer binding: " + path);
        }
        filters.push_back({ type, frequency, q, gain, !shared::protocol::Boolean(read(path + "bypass")) });
    }
    return filters;
}

} // namespace

void AnalyzerViewModel::Clear()
{
    qDragValue_.reset();
    preview_.clear();
    ready_ = false;
    context_.clear();
    entries_.clear();
    filters_.clear();
    curves_ = {};
    ++curvesRevision_;
    for (auto& bank : bankEntries_)
    {
        bank.clear();
    }
    allBanksCurve_.reset();
    otherBanksCurve_.reset();
    otherBanksActive_ = false;
    spectrum_.Clear();
}

void AnalyzerViewModel::SetSnapshot(const std::string& context,
    std::vector<shared::clients::ParameterEntry> entries, double sampleRate, int bank)
{
    qDragValue_.reset();
    preview_.clear();
    spectrum_.SetSampleRate(sampleRate);
    context_ = context;
    focusedBank_ = bank;
    const auto prefix = context == "equalizer" ? std::string("equalizer.filter.") : context + ".detector.filter.";
    std::erase_if(entries, [&prefix](const auto& entry)
        { return !entry.path.starts_with(prefix) && entry.path != "equalizer.bank.bypass"; });
    entries_ = std::move(entries);
    sampleRate_ = sampleRate;
    Rebuild();
    RebuildAllBanks();
}

const shared::clients::ParameterEntry& AnalyzerViewModel::Require(const std::string& path) const
{
    const auto found = std::find_if(entries_.begin(), entries_.end(),
        [&path](const auto& entry) { return entry.path == path; });
    if (found == entries_.end())
    {
        throw std::invalid_argument("Missing analyzer binding: " + path);
    }
    return *found;
}

double AnalyzerViewModel::Value(const std::string& path) const
{
    const auto found = std::find_if(preview_.begin(), preview_.end(),
        [&path](const auto& entry) { return entry.path == path; });
    return found == preview_.end() ? shared::protocol::Number(Require(path).value) : shared::protocol::Number(found->value);
}

std::pair<double, double> AnalyzerViewModel::Range(const std::string& path) const
{
    const auto& entry = Require(path);
    const auto minimum = groupScope_ ? entry.effectiveMinimum : entry.minimum;
    const auto maximum = groupScope_ ? entry.effectiveMaximum : entry.maximum;
    if (!minimum || !maximum || *maximum < *minimum)
    {
        throw std::invalid_argument("Invalid analyzer editing range: " + path);
    }
    return { *minimum, *maximum };
}

std::string AnalyzerViewModel::FilterPath(std::size_t index) const
{
    const std::string device = context_ == "equalizer" ? "equalizer" : context_ + ".detector";
    return device + ".filter." + std::to_string(index + 1);
}

void AnalyzerViewModel::Preview(const std::string& path, double value)
{
    const auto found = std::find_if(preview_.begin(), preview_.end(),
        [&path](const auto& entry) { return entry.path == path; });
    if (found == preview_.end())
    {
        preview_.push_back({ path, value });
    }
    else
    {
        found->value = value;
    }
}

void AnalyzerViewModel::SetGroupScope(bool groupScope)
{
    groupScope_ = groupScope;
}

void AnalyzerViewModel::FinishPreview()
{
    qDragValue_.reset();
    preview_.clear();
    Rebuild();
}

void AnalyzerViewModel::ApplyPreview(std::vector<features::equalizer::bindings::ParameterEdit> values)
{
    preview_ = std::move(values);
    Rebuild();
}

void AnalyzerViewModel::Apply(const shared::clients::ParameterEntry& entry)
{
    const auto found = std::find_if(entries_.begin(), entries_.end(),
        [&entry](const auto& current) { return current.path == entry.path; });
    if (found != entries_.end())
    {
        *found = entry;
        Rebuild();
    }
}

void AnalyzerViewModel::SetSampleRate(double sampleRate)
{
    if (sampleRate_ == sampleRate)
    {
        return;
    }
    sampleRate_ = sampleRate;
    spectrum_.SetSampleRate(sampleRate);
    Rebuild();
    RebuildAllBanks();
}

void AnalyzerViewModel::Rebuild()
{
    ++curvesRevision_;
    ready_ = false;
    filters_.clear();
    if (entries_.empty() || !std::isfinite(sampleRate_) || sampleRate_ <= 0)
    {
        allBanksCurve_.reset();
        return;
    }
    const bool equalizer = context_ == "equalizer";
    const std::span<const features::equalizer::presentation::AnalyzerFilterType> types = equalizer
        ? std::span<const features::equalizer::presentation::AnalyzerFilterType>(kEqualizerTypes)
        : std::span<const features::equalizer::presentation::AnalyzerFilterType>(kDetectorTypes);
    const auto prefix = equalizer ? "equalizer.filter." : context_ + ".detector.filter.";
    bankActive_ = !equalizer || !shared::protocol::Boolean(Require("equalizer.bank.bypass").value);
    for (std::size_t index = 0; index < types.size(); ++index)
    {
        const auto path = prefix + std::to_string(index + 1) + ".";
        const auto type = types[index];
        const auto gainRange = Range(path + "gain");
        (void)gainRange;
        if (type != features::equalizer::presentation::AnalyzerFilterType::Gain)
        {
            const auto frequencyRange = Range(path + "frequency");
            if (frequencyRange.first <= 0)
            {
                throw std::invalid_argument("Invalid analyzer frequency range: " + path);
            }
        }
        if (type == features::equalizer::presentation::AnalyzerFilterType::Bell)
        {
            const auto qRange = Range(path + "q");
            if (qRange.first <= 0)
            {
                throw std::invalid_argument("Invalid analyzer Q range: " + path);
            }
        }
    }
    filters_ = ReadFilters(context_, [this](const std::string& path) -> shared::protocol::ProtocolValue
    {
        return path.ends_with(".bypass") ? Require(path).value : shared::protocol::ProtocolValue{ Value(path) };
    });
    curves_ = features::equalizer::presentation::AnalyzerCurveCalculator::Calculate(filters_, sampleRate_, bankActive_);
    ready_ = true;
    ComposeAllBanks();
}

void AnalyzerViewModel::ApplyBankSnapshots(features::equalizer::clients::BankSnapshots banks)
{
    bankEntries_ = std::move(banks);
    RebuildAllBanks();
}

void AnalyzerViewModel::RebuildAllBanks()
{
    ++curvesRevision_;
    allBanksCurve_.reset();
    otherBanksCurve_.reset();
    otherBanksActive_ = false;
    if (context_ != "equalizer" || !std::isfinite(sampleRate_) || sampleRate_ <= 0 || bankEntries_[0].empty())
    {
        return;
    }
    features::equalizer::presentation::AnalyzerCurves::Curve combined;
    combined.fill(0.5);
    for (std::size_t bank = 0; bank < bankEntries_.size(); ++bank)
    {
        if (static_cast<int>(bank) == focusedBank_)
        {
            continue;
        }
        const auto& entries = bankEntries_[bank];
        const auto read = [&entries](const std::string& path) -> const shared::protocol::ProtocolValue&
        {
            const auto found = std::find_if(entries.begin(), entries.end(),
                [&path](const auto& entry) { return entry.path == path; });
            if (found == entries.end())
            {
                throw std::invalid_argument("Missing analyzer bank binding: " + path);
            }
            return found->value;
        };
        const auto filters = ReadFilters("equalizer", read);
        const bool bankActive = !shared::protocol::Boolean(read("equalizer.bank.bypass"));
        otherBanksActive_ = otherBanksActive_ || bankActive;
        const auto curves = features::equalizer::presentation::AnalyzerCurveCalculator::Calculate(filters, sampleRate_,
            bankActive);
        for (std::size_t point = 0; point < combined.size(); ++point)
        {
            combined[point] += curves.combined[point] - 0.5;
        }
    }
    otherBanksCurve_ = combined;
    ComposeAllBanks();
}

void AnalyzerViewModel::ComposeAllBanks()
{
    allBanksCurve_.reset();
    if (!ready_ || context_ != "equalizer" || !otherBanksCurve_ || (!otherBanksActive_ && !bankActive_))
    {
        return;
    }
    auto& combined = allBanksCurve_.emplace();
    for (std::size_t point = 0; point < combined.size(); ++point)
    {
        // Replace the focused snapshot contribution with the same accepted/preview
        // response used by the main curve. The renderer clips the resulting path.
        combined[point] = (*otherBanksCurve_)[point] + curves_.combined[point] - 0.5;
    }
}

void AnalyzerViewModel::SetSpectrumResolution(std::size_t pointCount) { spectrum_.SetResolution(pointCount); }

std::uint64_t AnalyzerViewModel::CurvesRevision() const { return curvesRevision_; }

void AnalyzerViewModel::ApplySpectrum(clients::AnalyzerSpectrum spectrum)
{
    if (ready_)
    {
        spectrum_.Apply(std::move(spectrum));
    }
}

void AnalyzerViewModel::BeginPreview(std::size_t index)
{
    const auto filter = filters_.at(index);
    const auto path = FilterPath(index);
    if (filter.type != features::equalizer::presentation::AnalyzerFilterType::Gain)
    {
        Preview(path + ".frequency", filter.frequency);
    }
    Preview(path + ".gain", filter.gain);
    qDragValue_ = filter.type == features::equalizer::presentation::AnalyzerFilterType::Bell
        ? std::optional<double>{ filter.q } : std::optional<double>{};
}

void AnalyzerViewModel::Move(std::size_t index, double x, double y)
{
    const auto path = FilterPath(index);
    const auto [minimumGain, maximumGain] = Range(path + ".gain");
    const double gain = std::clamp(presentation::AnalyzerScale::Gain(y), minimumGain, maximumGain);
    Preview(path + ".gain", gain);
    if (filters_.at(index).type != features::equalizer::presentation::AnalyzerFilterType::Gain)
    {
        const auto [minimum, maximum] = Range(path + ".frequency");
        const double frequency = std::clamp(presentation::AnalyzerScale::Frequency(std::clamp(x, 0.0, 1.0)), minimum, maximum);
        Preview(path + ".frequency", frequency);
    }
    Rebuild();
}

void AnalyzerViewModel::ChangeQ(std::size_t index, double delta)
{
    const auto path = FilterPath(index) + ".q";
    const auto [minimum, maximum] = Range(path);
    if (minimum <= 0 || maximum <= minimum || !qDragValue_ ||
        *qDragValue_ < minimum - 1e-9 || *qDragValue_ > maximum + 1e-9)
    {
        return;
    }
    const double current = std::clamp(*qDragValue_, minimum, maximum);
    const double next = std::clamp(current + delta * kQDragSensitivity, minimum, maximum);
    qDragValue_ = next;
    Preview(path, next);
    Rebuild();
}

bool AnalyzerViewModel::Ready() const { return ready_; }
bool AnalyzerViewModel::GroupScope() const { return groupScope_; }
bool AnalyzerViewModel::BankActive() const { return bankActive_; }
const std::vector<features::equalizer::bindings::ParameterEdit>& AnalyzerViewModel::PreviewValues() const { return preview_; }
const std::vector<features::equalizer::presentation::AnalyzerFilter>& AnalyzerViewModel::Filters() const { return filters_; }
const features::equalizer::presentation::AnalyzerCurves& AnalyzerViewModel::Curves() const { return curves_; }
const std::optional<features::equalizer::presentation::AnalyzerCurves::Curve>& AnalyzerViewModel::AllBanksCurve() const { return allBanksCurve_; }
const std::vector<double>& AnalyzerViewModel::SourceSpectrum() const { return spectrum_.Source(); }
const std::vector<double>& AnalyzerViewModel::GroupSpectrum() const { return spectrum_.Group(); }
const std::vector<double>& AnalyzerViewModel::ReferenceSpectrum() const { return spectrum_.Reference(); }
bool AnalyzerViewModel::HasSourceSpectrum() const { return !spectrum_.Source().empty(); }
bool AnalyzerViewModel::HasGroupSpectrum() const { return !spectrum_.Group().empty(); }

} // namespace consolidator::vst3::features::equalizer::viewmodels
