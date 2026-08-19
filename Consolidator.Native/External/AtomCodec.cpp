#include "AtomCodec.h"

namespace consolidator::max
{

std::optional<std::vector<NativeAtom>> AtomCodec::Encode(
    const c74::min::atoms& atoms)
{
    std::vector<NativeAtom> result;
    result.reserve(atoms.size());

    for (const auto& atom : atoms)
    {
        NativeAtom encoded{};

        switch (atom.a_type)
        {
        case c74::max::A_LONG:
            encoded.type = NativeAtomType::Integer;
            encoded.integer = static_cast<std::int64_t>(atom);
            break;

        case c74::max::A_FLOAT:
            encoded.type = NativeAtomType::Float;
            encoded.floating = static_cast<double>(atom);
            break;

        case c74::max::A_SYM:
            encoded.type = NativeAtomType::Symbol;
            encoded.symbol = c74::min::symbol(atom).c_str();
            break;

        default:
            return std::nullopt;
        }

        result.push_back(encoded);
    }

    return result;
}

} // namespace consolidator::max