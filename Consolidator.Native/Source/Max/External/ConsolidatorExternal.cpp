#include "ConsolidatorExternal.h"

#include <cstddef>
#include <utility>

namespace consolidator::max
{
using namespace c74::min;

ConsolidatorExternal::ConsolidatorExternal()
    : responseQueue_(this, MIN_FUNCTION { DrainResponses(); return {}; })
{
    (void)instance_.SetResponseNotifier([this] { NotifyResponseAvailable(); });
    instance_.Initialize();
}

ConsolidatorExternal::~ConsolidatorExternal()
{
    acceptingResponses_.store(false, std::memory_order_release);
    instance_.ShutdownResponseNotifier();
    responseQueue_.unset();
}

void ConsolidatorExternal::operator()(audio_bundle input, audio_bundle output)
{
    // The audio boundary contains no protocol or coordinator work.
    instance_.Process(
        input.samples(0),
        input.samples(1),
        input.samples(2),
        input.samples(3),
        output.samples(0),
        output.samples(1),
        output.samples(2),
        output.samples(3),
        static_cast<std::size_t>(input.frame_count()));
}

void ConsolidatorExternal::HandleProtocolMessage(symbol selector, const atoms& args)
{
    const auto result = protocol_.Decode(selector, args, instance_.GetInstanceId());
    if (result.command)
    {
        std::visit(
            [this](auto&& command)
            {
                instance_.EnqueueCommand(std::move(command));
            },
            std::move(*result.command));
    }
    if (result.error)
    {
        EmitProtocolError(*result.error);
    }
}

void ConsolidatorExternal::NotifyResponseAvailable()
{
    if (!acceptingResponses_.load(std::memory_order_acquire))
    {
        return;
    }
    bool expected = false;
    if (responseDispatchPending_.compare_exchange_strong(
            expected, true, std::memory_order_acq_rel))
    {
        responseQueue_.set();
    }
}

void ConsolidatorExternal::DrainResponses()
{
    while (auto response = instance_.TryDequeueResponse())
    {
        protocol_.EncodeResponse(
            *response,
            [this](symbol selector, const atoms& values)
            {
                atoms frame{atom{selector}};
                frame.insert(frame.end(), values.begin(), values.end());
                controlOutput.send(frame);
            });
    }

    responseDispatchPending_.store(false, std::memory_order_release);
    if (instance_.HasResponse())
    {
        responseDispatchPending_.store(true, std::memory_order_release);
        responseQueue_.set();
    }
}

void ConsolidatorExternal::HandleAnalysisTick(const atoms& args)
{
    if (args.size() != 1 || args[0].a_type != c74::max::A_LONG)
    {
        return;
    }

    const auto publicBankId = static_cast<int>(args[0]);
    if (publicBankId < 1 || publicBankId > 7)
    {
        return;
    }

    analysis::AnalysisService::Get().SetView({
        instance_.GetInstanceId(),
        static_cast<dsp::BankId>(publicBankId - 1)});
    EmitLatestAnalysis();
}

void ConsolidatorExternal::EmitLatestAnalysis()
{
    auto& analysisService = analysis::AnalysisService::Get();

    analysis::SpectrumSnapshot spectrum;
    if (analysisService.TryReadLatestSpectrum(
            spectrum, lastSpectrumRevision_))
    {
        lastSpectrumRevision_ = spectrum.revision;
        EmitSpectrum(symbol("spectrum_main"), spectrum);
    }

    analysis::SpectrumSnapshot reference;
    if (analysisService.TryReadLatestReferenceSpectrum(
            reference, lastReferenceSpectrumRevision_))
    {
        lastReferenceSpectrumRevision_ = reference.revision;
        EmitSpectrum(symbol("spectrum_reference"), reference);
    }

    analysis::SpectrumSnapshot difference;
    if (analysisService.TryReadLatestDifferenceSpectrum(
            difference, lastDifferenceSpectrumRevision_))
    {
        lastDifferenceSpectrumRevision_ = difference.revision;
        EmitSpectrum(symbol("spectrum_difference"), difference);
    }

    analysis::EqualizerCurveSnapshot curves;
    if (analysisService.TryReadLatestCurve(curves, lastCurveRevision_))
    {
        lastCurveRevision_ = curves.revision;
        EmitCurves(curves);
    }
}

void ConsolidatorExternal::EmitSpectrum(
    symbol selector,
    const analysis::SpectrumSnapshot& snapshot)
{
    atoms frame{atom{selector}};
    frame.reserve(frame.size() + snapshot.magnitudeDb.size());
    for (const auto value : snapshot.magnitudeDb)
    {
        frame.emplace_back(atom{static_cast<double>(value)});
    }
    analysisOutput.send(frame);
}

void ConsolidatorExternal::EmitCurves(
    const analysis::EqualizerCurveSnapshot& snapshot)
{
    for (std::size_t index = 0; index < snapshot.filters.size(); ++index)
    {
        atoms frame{atom{symbol("eq_filter")}, atom{static_cast<int>(index + 1)}};
        frame.reserve(frame.size() + snapshot.filters[index].magnitudeDb.size());
        for (const auto value : snapshot.filters[index].magnitudeDb)
        {
            frame.emplace_back(atom{static_cast<double>(value)});
        }
        analysisOutput.send(frame);
    }

    const auto emitAggregate = [this](symbol selector,
                                       const analysis::FrequencyResponseSnapshot& curve)
    {
        atoms frame{atom{selector}};
        frame.reserve(frame.size() + curve.magnitudeDb.size());
        for (const auto value : curve.magnitudeDb)
        {
            frame.emplace_back(atom{static_cast<double>(value)});
        }
        analysisOutput.send(frame);
    };
    emitAggregate(symbol("eq_combined"), snapshot.combined);
    emitAggregate(symbol("eq_all_banks"), snapshot.allBanksCombined);
}

void ConsolidatorExternal::EmitProtocolError(const ProtocolError& error)
{
    protocol_.EmitError(
        error,
        [this](symbol selector, const atoms& values)
        {
            atoms frame{atom{selector}};
            frame.insert(frame.end(), values.begin(), values.end());
            controlOutput.send(frame);
        });
}

} // namespace consolidator::max

MIN_EXTERNAL(consolidator::max::ConsolidatorExternal);
