#include "ConsolidatorExternal.h"

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
        input.samples(1),
        input.samples(2),
        input.samples(3),
        input.samples(4),
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
