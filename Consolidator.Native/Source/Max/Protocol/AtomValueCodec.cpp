#include "AtomValueCodec.h"

#include <string>
#include <type_traits>

#include "WireIdCodec.h"

namespace consolidator::max
{
namespace
{
using namespace c74::min;

std::string Text(const atom& value)
{
    return static_cast<std::string>(value);
}

std::optional<std::size_t> PositiveIndex(const atom& value, std::size_t max)
{
    if (value.a_type != c74::max::A_LONG)
    {
        return std::nullopt;
    }
    const auto index = static_cast<long long>(value);
    if (index < 1 || static_cast<std::size_t>(index) > max)
    {
        return std::nullopt;
    }
    return static_cast<std::size_t>(index - 1);
}
} // namespace

std::optional<core::StateValue> AtomValueCodec::Decode(
    const atom& value, const core::StatePath& path) const
{
    if (!path.field)
    {
        return std::nullopt;
    }
    switch (*path.field)
    {
    case core::StateField::SelectedBank:
    {
        std::optional<std::size_t> index = PositiveIndex(value, 7);
        if (!index && value.a_type == c74::max::A_SYM)
        {
            const auto text = Text(value);
            if (text.size() == 5 &&
                text.substr(0, 4) == "bank" &&
                text[4] >= '1' &&
                text[4] <= '7')
            {
                index = static_cast<std::size_t>(text[4] - '1');
            }
        }
        if (!index)
        {
            return std::nullopt;
        }
        return core::StateValue{static_cast<dsp::BankId>(*index)};
    }
    case core::StateField::GroupId:
        if (value.a_type == c74::max::A_SYM && Text(value) == "none")
        {
            return core::StateValue{std::monostate{}};
        }
        if (value.a_type == c74::max::A_LONG &&
            static_cast<long long>(value) >= 0)
        {
            return core::StateValue{core::GroupId{
                static_cast<core::GroupId::ValueType>(value)}};
        }
        return std::nullopt;
    case core::StateField::DspParameter:
        if (value.a_type == c74::max::A_LONG ||
            value.a_type == c74::max::A_FLOAT)
        {
            return core::StateValue{static_cast<float>(value)};
        }
        return std::nullopt;
    case core::StateField::DspMarker:
    case core::StateField::Mute:
    case core::StateField::Solo:
        if (value.a_type == c74::max::A_LONG)
        {
            const auto integer = static_cast<long long>(value);
            if (integer == 0 || integer == 1)
            {
                return core::StateValue{integer == 1};
            }
        }
        return std::nullopt;
    default:
        return std::nullopt;
    }
}

void AtomValueCodec::Encode(atoms& output, const core::StateValue& value) const
{
    std::visit(
        [&](const auto& typed)
        {
            using T = std::decay_t<decltype(typed)>;
            if constexpr (std::is_same_v<T, std::monostate>)
            {
                output.emplace_back("none");
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                output.emplace_back(typed ? 1 : 0);
            }
            else if constexpr (std::is_same_v<T, dsp::BankId>)
            {
                output.emplace_back(
                    "bank" + std::to_string(static_cast<int>(typed) + 1));
            }
            else if constexpr (std::is_same_v<T, core::GroupId>)
            {
                output.emplace_back(static_cast<long long>(typed.GetValue()));
            }
            else if constexpr (std::is_same_v<T, core::InstanceId>)
            {
                output.emplace_back(EncodeWireId(typed.GetValue()));
            }
            else
            {
                output.emplace_back(typed);
            }
        },
        value);
}
} // namespace consolidator::max
