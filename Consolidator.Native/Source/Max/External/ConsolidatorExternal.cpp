#include "ConsolidatorExternal.h"

#include "../Protocol/BankIdCodec.h"
#include "../Protocol/WireIdCodec.h"

#include <cstddef>
#include <utility>

namespace consolidator::max
{
using namespace c74::min;

ConsolidatorExternal::ConsolidatorExternal()
    : responseQueue_(this, MIN_FUNCTION { DrainResponses(); return {}; })
{
    (void)instance_.SetResponseNotifier([this] { NotifyResponseAvailable(); });
    (void)instance_.SetRegistryNotifier(
        [this](std::uint64_t revision) { NotifyRegistryChanged(revision); });
    instance_.Initialize();
}

ConsolidatorExternal::~ConsolidatorExternal()
{
    acceptingResponses_.store(false, std::memory_order_release);
    instance_.ShutdownNotifiers();
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

void ConsolidatorExternal::NotifyRegistryChanged(std::uint64_t revision)
{
    auto current = pendingRegistryRevision_.load(std::memory_order_relaxed);
    while (current < revision &&
           !pendingRegistryRevision_.compare_exchange_weak(
               current, revision, std::memory_order_release,
               std::memory_order_relaxed))
    {
    }
    NotifyResponseAvailable();
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

    const auto registryRevision = pendingRegistryRevision_.load(
        std::memory_order_acquire);
    if (registryRevision > emittedRegistryRevision_)
    {
        emittedRegistryRevision_ = registryRevision;
        controlOutput.send(atoms{
            atom{symbol("registry_changed")},
            atom{kProtocolVersion},
            EncodeWireId(registryRevision)});
    }

    responseDispatchPending_.store(false, std::memory_order_release);
    const bool hasPendingRegistry =
        pendingRegistryRevision_.load(std::memory_order_acquire) >
        emittedRegistryRevision_;
    if (instance_.HasResponse() || hasPendingRegistry)
    {
        bool expected = false;
        if (responseDispatchPending_.compare_exchange_strong(
                expected,
                true,
                std::memory_order_acq_rel))
        {
            responseQueue_.set();
        }
    }
}

void ConsolidatorExternal::HandleAnalysisView(const atoms& args)
{
    if (args.size() != 2 ||
        args[1].a_type != c74::max::A_LONG)
    {
        return;
    }

    const auto instanceValue = DecodeWireId(args[0]);
    const auto publicBankId = static_cast<int>(args[1]);
    if (!instanceValue)
    {
        return;
    }
    const auto bankId = DecodeBankId(publicBankId);
    if (!bankId)
    {
        return;
    }

    analysis::AnalysisService::Get().SetView({
        core::InstanceId{static_cast<core::InstanceId::ValueType>(*instanceValue)},
        *bankId});
    ResetAnalysisRevisions();
}

void ConsolidatorExternal::HandleAnalysisTick(const atoms& args)
{
    if (!args.empty())
    {
        return;
    }

    EmitLatestAnalysis();
}

void ConsolidatorExternal::ResetAnalysisRevisions() noexcept
{
    lastSpectrumRevision_ = 0;
    lastReferenceSpectrumRevision_ = 0;
    lastDifferenceSpectrumRevision_ = 0;
    lastCurveRevision_ = 0;
    lastTelemetryRevision_ = 0;
    lastTelemetryViewRevision_ = 0;
}

void ConsolidatorExternal::EmitLatestAnalysis()
{
    auto& analysisService = analysis::AnalysisService::Get();
    analysis::AnalysisView view;

    analysis::SpectrumSnapshot spectrum;
    if (analysisService.TryReadLatestSpectrum(
            spectrum, lastSpectrumRevision_, view))
    {
        lastSpectrumRevision_ = spectrum.revision;
        EmitSpectrum(symbol("spectrum_main"), spectrum, view);
    }

    analysis::SpectrumSnapshot reference;
    if (analysisService.TryReadLatestReferenceSpectrum(
            reference, lastReferenceSpectrumRevision_, view))
    {
        lastReferenceSpectrumRevision_ = reference.revision;
        EmitSpectrum(symbol("spectrum_reference"), reference, view);
    }

    analysis::SpectrumSnapshot difference;
    if (analysisService.TryReadLatestDifferenceSpectrum(
            difference, lastDifferenceSpectrumRevision_, view))
    {
        lastDifferenceSpectrumRevision_ = difference.revision;
        EmitSpectrum(symbol("spectrum_difference"), difference, view);
    }

    analysis::EqualizerCurveSnapshot curves;
    if (analysisService.TryReadLatestCurve(curves, lastCurveRevision_, view))
    {
        lastCurveRevision_ = curves.revision;
        EmitCurves(curves, view);
    }

    dsp::TelemetrySnapshot telemetry;
    if (analysisService.TryReadLatestTelemetry(
            telemetry,
            lastTelemetryRevision_,
            lastTelemetryViewRevision_,
            view))
    {
        lastTelemetryRevision_ = telemetry.revision;
        lastTelemetryViewRevision_ = telemetry.viewRevision;
        EmitTelemetry(telemetry, view);
    }
}

void ConsolidatorExternal::EmitSpectrum(
    symbol selector,
    const analysis::SpectrumSnapshot& snapshot,
    const analysis::AnalysisView& view)
{
    atoms frame{
        atom{selector},
        EncodeWireId(snapshot.viewRevision),
        EncodeWireId(view.instanceId.GetValue()),
        atom{static_cast<c74::max::t_atom_long>(
            EncodeBankId(view.bankId))}};
    frame.reserve(frame.size() + snapshot.magnitudeDb.size());
    for (const auto value : snapshot.magnitudeDb)
    {
        frame.emplace_back(atom{static_cast<double>(value)});
    }
    analysisOutput.send(frame);
}

void ConsolidatorExternal::EmitCurves(
    const analysis::EqualizerCurveSnapshot& snapshot,
    const analysis::AnalysisView& view)
{
    for (std::size_t index = 0; index < snapshot.filters.size(); ++index)
    {
        atoms frame{
            atom{symbol("eq_filter")},
            EncodeWireId(snapshot.viewRevision),
            EncodeWireId(view.instanceId.GetValue()),
            atom{static_cast<c74::max::t_atom_long>(
                EncodeBankId(view.bankId))},
            atom{static_cast<int>(index + 1)}};
        frame.reserve(frame.size() + snapshot.filters[index].magnitudeDb.size());
        for (const auto value : snapshot.filters[index].magnitudeDb)
        {
            frame.emplace_back(atom{static_cast<double>(value)});
        }
        analysisOutput.send(frame);
    }

    const auto emitAggregate = [this, &view](symbol selector,
                                             const analysis::FrequencyResponseSnapshot& curve)
    {
        atoms frame{
            atom{selector},
            EncodeWireId(curve.viewRevision),
            EncodeWireId(view.instanceId.GetValue()),
            atom{static_cast<c74::max::t_atom_long>(
            EncodeBankId(view.bankId))}};
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

void ConsolidatorExternal::EmitTelemetry(
    const dsp::TelemetrySnapshot& snapshot,
    const analysis::AnalysisView& view)
{
    using dsp::MeterPoint;
    using dsp::ToIndex;

    const auto emitMeter = [this, &snapshot, &view](symbol point,
                                                    const dsp::LevelTelemetry& meter)
    {
        analysisOutput.send(atoms{
            atom{symbol("meter")},
            EncodeWireId(snapshot.viewRevision),
            EncodeWireId(view.instanceId.GetValue()),
            atom{static_cast<c74::max::t_atom_long>(
                EncodeBankId(view.bankId))},
            atom{point}, atom{meter.rmsDb}, atom{meter.peakDb},
            atom{meter.smoothedDb}});
    };
    emitMeter(
        symbol("input_gain"),
        snapshot.levels[ToIndex(MeterPoint::InputGainOutput)]);
    emitMeter(
        symbol("saturator"),
        snapshot.levels[ToIndex(MeterPoint::SaturatorOutput)]);
    emitMeter(
        symbol("compressor"),
        snapshot.levels[ToIndex(MeterPoint::CompressorOutput)]);
    emitMeter(
        symbol("output_gain"),
        snapshot.levels[ToIndex(MeterPoint::OutputGainOutput)]);

    analysisOutput.send(atoms{
        atom{symbol("saturator_distortion")},
        EncodeWireId(snapshot.viewRevision),
        EncodeWireId(view.instanceId.GetValue()),
        atom{static_cast<c74::max::t_atom_long>(
            EncodeBankId(view.bankId))},
        atom{snapshot.saturator.distortionPercent},
        atom{snapshot.saturator.distortionSmoothedPercent}});
    analysisOutput.send(atoms{
        atom{symbol("compressor_reduction")},
        EncodeWireId(snapshot.viewRevision),
        EncodeWireId(view.instanceId.GetValue()),
        atom{static_cast<c74::max::t_atom_long>(
            EncodeBankId(view.bankId))},
        atom{snapshot.compressor.gainReductionRmsDb},
        atom{snapshot.compressor.gainReductionPeakDb},
        atom{snapshot.compressor.gainReductionSmoothedDb}});
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
