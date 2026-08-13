#include "MaxProtocolAdapter.h"

#include <cstdint>

#include "AtomCommandDecoder.h"
#include "AtomResponseEncoder.h"
#include "WireIdCodec.h"
#include "BankIdCodec.h"

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
    const auto framingError = [&](const char* code, const char* message)
    {
        const auto source = args.size() > 1 && IsSymbol(args[1])
            ? symbol(args[1])
            : symbol("");
        const auto wire = args.size() > 2
            ? DecodeWireId(args[2])
            : std::nullopt;
        return DecodeResult{
            std::nullopt,
            ProtocolError{
                code,
                message,
                source,
                wire.value_or(0),
                instance}};
    };

    if (args.size() < 3 || !IsInteger(args[0]) || !IsSymbol(args[1]))
    {
        return framingError("malformed", "invalid frame");
    }

    if (static_cast<int>(args[0]) != kProtocolVersion)
    {
        return framingError("unsupported_version", "unsupported protocol version");
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
    const auto terminalSelector =
        std::holds_alternative<core::ActionResponse>(response)
            ? symbol("action_done")
            : std::holds_alternative<core::RegistryResponse>(response)
                ? symbol("registry_done")
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

    if (const auto* registry = std::get_if<core::RegistryResponse>(&response))
    {
        EncodeRegistrySnapshot(
            registry->snapshot,
            source,
            wire,
            std::move(terminalAwareSink));
    }
    else
    {
        responseEncoder_->Encode(
            response,
            source,
            wire,
            std::move(terminalAwareSink));
    }
}

void MaxProtocolAdapter::EncodeRegistrySnapshot(
    const core::RegistrySnapshot& snapshot,
    symbol source,
    std::uint64_t wireRequestId,
    FrameSink sink) const
{
    const auto prefix = [&]()
    {
        return atoms{atom{kProtocolVersion}, atom{source}, EncodeWireId(wireRequestId)};
    };
    auto begin = prefix();
    begin.emplace_back(EncodeWireId(snapshot.revision));
    begin.emplace_back(static_cast<int>(snapshot.instances.size()));
    begin.emplace_back(static_cast<int>(snapshot.groups.size()));
    sink(symbol("registry_begin"), begin);

    for (const auto& instance : snapshot.instances)
    {
        auto frame = prefix();
        frame.emplace_back(EncodeWireId(instance.instanceId.GetValue()));
        frame.emplace_back(instance.label);
        frame.emplace_back(EncodeBankId(instance.selectedBankId));
        sink(symbol("registry_instance"), frame);
        for (const auto& bank : instance.banks)
        {
            frame = prefix();
            frame.emplace_back(EncodeWireId(instance.instanceId.GetValue()));
            frame.emplace_back(EncodeBankId(bank.bankId));
            frame.emplace_back(bank.groupId
                ? atom{static_cast<long long>(bank.groupId->GetValue())}
                : atom{symbol("none")});
            sink(symbol("registry_bank"), frame);
        }
    }

    for (const auto& group : snapshot.groups)
    {
        auto frame = prefix();
        frame.emplace_back(static_cast<long long>(group.groupId.GetValue()));
        sink(symbol("registry_group"), frame);
        for (const auto& member : group.members)
        {
            frame = prefix();
            frame.emplace_back(static_cast<long long>(group.groupId.GetValue()));
            frame.emplace_back(EncodeWireId(member.instanceId.GetValue()));
            frame.emplace_back(EncodeBankId(member.bankId));
            sink(symbol("registry_member"), frame);
        }
    }
    sink(symbol("registry_done"), prefix());
}

} // namespace consolidator::max
