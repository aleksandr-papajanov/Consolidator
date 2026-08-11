#include "MaxProtocolAdapter.h"

#include <cstdint>

#include "AtomCommandDecoder.h"
#include "AtomResponseEncoder.h"
#include "WireIdCodec.h"

namespace consolidator::max
{
namespace
{

using namespace c74::min;

bool IsSymbol(const atom& value)
{
    return value.a_type == c74::max::A_SYM;
}

bool IsInteger(const atom& value)
{
    return value.a_type == c74::max::A_LONG;
}

std::string Text(const atom& value)
{
    return static_cast<std::string>(value);
}

} // namespace

MaxProtocolAdapter::MaxProtocolAdapter()
    : commandDecoder_(std::make_unique<AtomCommandDecoder>()),
      responseEncoder_(std::make_unique<AtomResponseEncoder>())
{
}

MaxProtocolAdapter::~MaxProtocolAdapter() = default;

DecodeResult MaxProtocolAdapter::Decode(
    symbol selector,
    const atoms& args,
    core::InstanceId instance)
{
    if (args.size() < 3 || !IsInteger(args[0]) || !IsSymbol(args[1]))
    {
        return commandDecoder_->Decode(selector, args, instance, 0);
    }

    if (static_cast<int>(args[0]) != kProtocolVersion)
    {
        return commandDecoder_->Decode(selector, args, instance, 0);
    }

    const auto source = symbol(args[1]);
    const auto wire = DecodeWireId(args[2]);
    if (!wire)
    {
        return commandDecoder_->Decode(selector, args, instance, 0);
    }

    const WireRequestKey key{source, *wire};
    if (pending_.contains(key))
    {
        return DecodeResult{
            std::nullopt,
            ProtocolError{
                "duplicate_request",
                "request is already pending",
                source,
                *wire,
                instance}};
    }

    const auto coreRequestId = nextRequestId_++;
    auto result = commandDecoder_->Decode(
        selector, args, instance, coreRequestId);
    if (!result.command)
    {
        return result;
    }

    requests_[coreRequestId] = {source, *wire};
    pending_[key] = coreRequestId;
    return result;
}

void MaxProtocolAdapter::EmitError(const ProtocolError& error, FrameSink sink) const
{
    sink(symbol("error"), atoms{
        atom{kProtocolVersion}, atom{error.source},
        EncodeWireId(error.wireRequestId),
        EncodeWireId(error.instanceId.GetValue()),
        atom{error.code}, atom{error.message}});
}

void MaxProtocolAdapter::EncodeResponse(
    const core::CommandResponse& response,
    FrameSink sink)
{
    const auto coreRequestId = std::visit(
        [](const auto& item)
        {
            return item.requestId;
        },
        response);
    const auto request = requests_.find(coreRequestId);
    if (request == requests_.end())
    {
        return;
    }

    const auto source = request->second.source;
    const auto wire = request->second.wireRequestId;
    const auto terminalSelector = std::holds_alternative<core::ActionResponse>(
                                      response)
        ? symbol("action_done")
        : symbol("state_done");
    const WireRequestKey key{source, wire};
    auto terminalAwareSink =
        [this,
         source,
         wire,
         coreRequestId,
         terminalSelector,
         sink = std::move(sink)](symbol selector, const atoms& frame) mutable
    {
        if (selector == terminalSelector)
        {
            pending_.erase(WireRequestKey{source, wire});
            requests_.erase(coreRequestId);
        }
        sink(selector, frame);
    };

    responseEncoder_->Encode(
        response,
        source,
        wire,
        std::move(terminalAwareSink));
}

} // namespace consolidator::max
