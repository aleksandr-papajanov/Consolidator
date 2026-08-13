#include "AtomCommandDecoder.h"

#include <cstdint>
#include <string>
#include <utility>

#include "AtomPathCodec.h"
#include "AtomValueCodec.h"
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

DecodeResult AtomCommandDecoder::Decode(
    symbol selector,
    const atoms& args,
    core::InstanceId instance,
    core::RequestId requestId) const
{
    DecodeResult result;
    const auto verb = Text(atom{selector});
    const bool action = verb == "reset";
    const bool registry = verb == "registry";
    const auto fail = [&](const char* code, const char* message)
    {
        const auto source = args.size() > 1 && IsSymbol(args[1])
            ? symbol(args[1])
            : symbol("");
        const auto wire = args.size() > 2 ? DecodeWireId(args[2]) : std::nullopt;
        result.error = ProtocolError{
            code,
            message,
            source,
            wire.value_or(0),
            instance};
        return result;
    };

    if ((verb != "read" && verb != "write" && verb != "reset" &&
         verb != "registry") ||
        args.empty() || !IsInteger(args[0]))
    {
        return fail("malformed", "invalid frame");
    }

    if (static_cast<int>(args[0]) != kProtocolVersion)
    {
        return fail("unsupported_version", "unsupported protocol version");
    }

    if (args.size() < 3u || !IsSymbol(args[1]))
    {
        return fail("malformed", "invalid frame");
    }

    if (Text(args[1]).empty())
    {
        return fail("unknown_source", "source must be non-empty");
    }

    const auto source = symbol(args[1]);
    const auto wire = DecodeWireId(args[2]);
    if (!wire)
    {
        return fail("malformed", "request must be an unsigned decimal symbol");
    }

    if (registry)
    {
        if (args.size() != 3u)
        {
            return fail("malformed", "invalid registry request");
        }
        result.command = core::ReadRegistryCommand{
            requestId,
            instance};
        return result;
    }

    if (args.size() < 4u)
    {
        return fail("malformed", "invalid frame");
    }

    const auto invalidPath = [&]
    {
        return fail("invalid_path", "invalid state path");
    };
    const auto invalidValue = [&]
    {
        return fail("invalid_value", "value does not match state path");
    };
    auto parsePath = [&](std::size_t& position, std::size_t end)
    {
        return AtomPathCodec{}.Decode(args, position, end, instance);
    };

    if (!action)
    {
        if (!IsInteger(args[3]))
        {
            return fail("malformed", "invalid count");
        }
        const auto count = static_cast<int>(args[3]);
        if (count < 0 || count > 16)
        {
            return fail("batch_overflow", "too many entries");
        }
        std::size_t position = 4;

        if (verb == "read")
        {
            core::ReadStateCommand command{requestId, instance, {}};
            for (int index = 0; index < count; ++index)
            {
                if (position >= args.size() ||
                    Text(args[position++]) != "query")
                {
                    return invalidPath();
                }
                const auto pathBegin = position;
                while (position < args.size() &&
                       !(IsSymbol(args[position]) &&
                         Text(args[position]) == "query"))
                {
                    ++position;
                }
                auto pathPosition = pathBegin;
                const auto path = parsePath(pathPosition, position);
                if (!path || pathPosition != position)
                {
                    return invalidPath();
                }
                (void)command.queries.TryAppend(core::StateEntry{*path, {}});
            }
            if (position != args.size())
            {
                return fail("malformed", "extra atoms");
            }
            result.command = std::move(command);
            return result;
        }

        core::WriteStateCommand command{requestId, instance, {}};
        for (int index = 0; index < count; ++index)
        {
            if (position >= args.size() ||
                Text(args[position++]) != "entry")
            {
                return invalidPath();
            }
            const auto pathBegin = position;
            while (position < args.size() &&
                   !(IsSymbol(args[position]) &&
                     Text(args[position]) == "value"))
            {
                ++position;
            }
            if (position >= args.size())
            {
                return invalidPath();
            }
            auto pathPosition = pathBegin;
            const auto path = parsePath(pathPosition, position);
            if (!path || pathPosition != position)
            {
                return invalidPath();
            }
            ++position;
            if (position >= args.size())
            {
                return invalidValue();
            }
            const auto value = AtomValueCodec{}.Decode(args[position++], *path);
            if (!value)
            {
                return invalidValue();
            }
            (void)command.entries.TryAppend(core::StateEntry{*path, *value});
        }
        if (position != args.size())
        {
            return fail("malformed", "extra atoms");
        }
        result.command = std::move(command);
        return result;
    }

    std::size_t position = 3;
    const auto path = parsePath(position, args.size());
    if (!path || position != args.size() || !path->IsValidResetTarget())
    {
        return invalidPath();
    }
    result.command = core::ResetDspCommand{requestId, instance, *path};
    return result;
}

} // namespace consolidator::max
