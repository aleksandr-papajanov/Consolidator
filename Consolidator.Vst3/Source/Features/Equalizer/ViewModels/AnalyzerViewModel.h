#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Shared/Clients/AnalyzerData.h"
#include "Shared/Clients/ParameterEntry.h"
#include "Features/Equalizer/Bindings/ParameterEdit.h"
#include "Features/Equalizer/Presentation/AnalyzerCurveCalculator.h"

namespace consolidator::vst3::features::equalizer::viewmodels
{

class AnalyzerViewModel final
{
public:
    void Clear();
    void SetSnapshot(const std::string& context, std::vector<shared::clients::ParameterEntry> entries,
        double sampleRate, int bank);
    void Apply(const shared::clients::ParameterEntry& entry);
    void SetSampleRate(double sampleRate);
    void SetGroupScope(bool groupScope);
    void ApplyPreview(std::vector<features::equalizer::bindings::ParameterEdit> values);
    void FinishPreview();
    void Preview(const std::string& path, double value);
    void BeginPreview(std::size_t index);
    void Move(std::size_t index, double x, double y);
    void ChangeQ(std::size_t index, double delta);
    void ApplySpectrum(shared::clients::AnalyzerSpectrum spectrum);
    void ApplyBankSnapshots(shared::clients::BankSnapshots banks);
    void SetSpectrumResolution(std::size_t pointCount);
    [[nodiscard]] std::uint64_t CurvesRevision() const;
    [[nodiscard]] double Value(const std::string& path) const;
    [[nodiscard]] std::pair<double, double> Range(const std::string& path) const;
    [[nodiscard]] std::string FilterPath(std::size_t index) const;
    [[nodiscard]] bool Ready() const;
    [[nodiscard]] bool GroupScope() const;
    [[nodiscard]] bool BankActive() const;
    [[nodiscard]] const std::vector<features::equalizer::bindings::ParameterEdit>& PreviewValues() const;
    [[nodiscard]] const std::vector<features::equalizer::presentation::AnalyzerFilter>& Filters() const;
    [[nodiscard]] const features::equalizer::presentation::AnalyzerCurves& Curves() const;
    [[nodiscard]] const std::optional<features::equalizer::presentation::AnalyzerCurves::Curve>& AllBanksCurve() const;
    [[nodiscard]] const std::vector<double>& SourceSpectrum() const;
    [[nodiscard]] const std::vector<double>& GroupSpectrum() const;
    [[nodiscard]] const std::vector<double>& ReferenceSpectrum() const;
    [[nodiscard]] bool HasSourceSpectrum() const;
    [[nodiscard]] bool HasGroupSpectrum() const;

private:
    struct SpectrumInterval
    {
        double center;
        double left;
        double right;
    };

    void Rebuild();
    void RebuildAllBanks();
    void ComposeAllBanks();
    [[nodiscard]] const shared::clients::ParameterEntry& Require(const std::string& path) const;
    void Resample(const std::vector<double>& bins, std::vector<double>& result) const;

    std::vector<features::equalizer::bindings::ParameterEdit> preview_;
    std::string context_;
    std::vector<shared::clients::ParameterEntry> entries_;
    std::vector<features::equalizer::presentation::AnalyzerFilter> filters_;
    features::equalizer::presentation::AnalyzerCurves curves_;
    shared::clients::BankSnapshots bankEntries_;
    std::optional<features::equalizer::presentation::AnalyzerCurves::Curve> otherBanksCurve_;
    std::optional<features::equalizer::presentation::AnalyzerCurves::Curve> allBanksCurve_;
    int focusedBank_{};
    bool otherBanksActive_{};
    std::vector<double> sourceBins_;
    std::vector<double> groupBins_;
    std::vector<double> referenceBins_;
    std::vector<double> referenceSpectrum_;
    std::vector<double> sourceSpectrum_;
    std::vector<double> groupSpectrum_;
    std::vector<SpectrumInterval> spectrumIntervals_;
    std::uint64_t curvesRevision_{};
    double sampleRate_{};
    std::size_t fftSize_{};
    bool ready_{};
    bool bankActive_{};
    bool groupScope_{};
    std::optional<double> qDragValue_;
};

} // namespace consolidator::vst3::features::equalizer::viewmodels
