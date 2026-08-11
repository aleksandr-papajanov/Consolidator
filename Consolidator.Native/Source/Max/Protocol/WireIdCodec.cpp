#include "WireIdCodec.h"

#include <charconv>
#include <string>

namespace consolidator::max
{

c74::min::atom EncodeWireId(std::uint64_t value)
{
    return c74::min::atom{c74::min::symbol(std::to_string(value))};
}

std::optional<std::uint64_t> DecodeWireId(const c74::min::atom& value)
{
    if (value.a_type != c74::max::A_SYM)
    {
        return std::nullopt;
    }

    const auto text = static_cast<std::string>(value);
    if (text.empty() || (text.size() > 1 && text.front() == '0'))
    {
        return std::nullopt;
    }

    std::uint64_t result = 0;
    const auto parsed = std::from_chars(
        text.data(), text.data() + text.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() ||
        result > 9007199254740991ULL)
    {
        return std::nullopt;
    }
    return result;
}

} // namespace consolidator::max
