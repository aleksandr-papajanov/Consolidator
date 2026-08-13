#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "Core/Domain/Commands/StateProtocolCommands.h"
#include "Core/Registry/RegistryState.h"
#include "c74_min_api.h"

namespace consolidator::max
{

inline constexpr int kProtocolVersion = 1;

class AtomCommandDecoder;
class AtomResponseEncoder;

struct ProtocolError
{
    std::string code;
    std::string message;
    c74::min::symbol source;
    core::RequestId wireRequestId = 0;
    core::InstanceId instanceId{0};
};

struct DecodeResult
{
    std::optional<core::Command> command;
    std::optional<ProtocolError> error;
};

struct WireRequestKey
{
    c74::min::symbol source;
    std::uint64_t requestId = 0;

    friend bool operator==(const WireRequestKey&, const WireRequestKey&) = default;
};

struct WireRequestKeyHash
{
    std::size_t operator()(const WireRequestKey& key) const noexcept
    {
        const auto symbolPointer = static_cast<c74::max::t_symbol*>(key.source);
        const auto sourceHash = std::hash<const void*>{}(symbolPointer);
        const auto requestHash = std::hash<std::uint64_t>{}(key.requestId);
        return sourceHash ^ (requestHash + 0x9e3779b9u +
                             (sourceHash << 6u) + (sourceHash >> 2u));
    }
};

// Owns wire correlation while delegating command and response codecs.
class MaxProtocolAdapter
{
public:
    MaxProtocolAdapter();
    ~MaxProtocolAdapter();

    DecodeResult Decode(c74::min::symbol selector,
                        const c74::min::atoms& args,
                        core::InstanceId instance);

    using FrameSink = std::function<void(c74::min::symbol, const c74::min::atoms&)>;
    void EncodeResponse(const core::CommandResponse& response, FrameSink sink);
    void EmitError(const ProtocolError& error, FrameSink sink) const;
    void EncodeRegistrySnapshot(
        const core::RegistrySnapshot& snapshot,
        c74::min::symbol source,
        std::uint64_t wireRequestId,
        FrameSink sink) const;

private:
    struct Correlation
    {
        c74::min::symbol source;
        core::RequestId wireRequestId;
    };

    std::uint64_t nextRequestId_ = 1;
    std::unordered_map<core::RequestId, Correlation> requests_;
    std::unordered_map<WireRequestKey, core::RequestId, WireRequestKeyHash> pending_;
    std::unique_ptr<AtomCommandDecoder> commandDecoder_;
    std::unique_ptr<AtomResponseEncoder> responseEncoder_;
};

} // namespace consolidator::max
