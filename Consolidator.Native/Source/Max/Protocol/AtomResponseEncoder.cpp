#include "AtomResponseEncoder.h"

#include <type_traits>

#include "AtomPathCodec.h"
#include "AtomValueCodec.h"
#include "WireIdCodec.h"

namespace consolidator::max
{
namespace
{

using namespace c74::min;

const char* Status(core::StateWriteStatus value)
{
    switch (value)
    {
    case core::StateWriteStatus::NotHandled:
        return "not_handled";
    case core::StateWriteStatus::Applied:
        return "applied";
    case core::StateWriteStatus::Unchanged:
        return "unchanged";
    case core::StateWriteStatus::Rejected:
        return "rejected";
    }
    return "none";
}

void PutRange(atoms& output, const std::optional<dsp::ParameterVariant>& value)
{
    if (!value)
    {
        output.emplace_back("none");
        return;
    }
    std::visit(
        [&](const auto& item)
        {
            output.emplace_back(item);
        },
        *value);
}

} // namespace

void AtomResponseEncoder::Encode(
    const core::CommandResponse& response,
    symbol source,
    std::uint64_t wireRequestId,
    MaxProtocolAdapter::FrameSink sink) const
{
    const auto responseInstance = std::visit(
        [](const auto& item)
        {
            return item.instanceId;
        },
        response);

    std::visit(
        [&](const auto& item)
        {
            using ResponseType = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<ResponseType, core::StateResponse>)
            {
                sink(symbol("state_begin"), atoms{
                    atom{kProtocolVersion}, atom{source},
                    EncodeWireId(wireRequestId),
                    EncodeWireId(responseInstance.GetValue()),
                    atom{item.truncated ? 1 : 0},
                    atom{static_cast<int>(item.entries.size)}});

                for (std::size_t index = 0; index < item.entries.size; ++index)
                {
                    const auto& entry = item.entries.entries[index];
                    const auto targetInstance = entry.path.instanceId.value_or(
                        responseInstance);
                    atoms frame{
                        atom{kProtocolVersion}, atom{source},
                        EncodeWireId(wireRequestId),
                        EncodeWireId(targetInstance.GetValue()),
                        atom{static_cast<int>(index)}};
                    AtomPathCodec{}.Encode(frame, entry.path);
                    AtomValueCodec{}.Encode(frame, entry.value);
                    frame.emplace_back(
                        entry.status ? Status(*entry.status) : "none");
                    PutRange(frame, entry.physicalMinimum);
                    PutRange(frame, entry.physicalMaximum);
                    PutRange(frame, entry.minimum);
                    PutRange(frame, entry.maximum);
                    sink(symbol("state_entry"), frame);
                }

                sink(symbol("state_done"), atoms{
                    atom{kProtocolVersion}, atom{source},
                    EncodeWireId(wireRequestId),
                    EncodeWireId(responseInstance.GetValue())});
            }
            else
            {
                sink(symbol("action_done"), atoms{
                    atom{kProtocolVersion}, atom{source},
                    EncodeWireId(wireRequestId),
                    EncodeWireId(responseInstance.GetValue()),
                    atom{item.status == core::ActionStatus::Accepted
                             ? "accepted" : "rejected"}});
            }
        },
        response);
}

} // namespace consolidator::max
