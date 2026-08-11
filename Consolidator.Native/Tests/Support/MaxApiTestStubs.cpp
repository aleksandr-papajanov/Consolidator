#include "c74_min_api.h"

#include <string>
#include <vector>

namespace c74::max
{
namespace
{

struct SymbolEntry
{
    std::string text;
    t_symbol symbol{};
};

std::vector<SymbolEntry>& Symbols()
{
    static auto* symbols = []
    {
        auto* value = new std::vector<SymbolEntry>();
        value->reserve(256);
        return value;
    }();
    return *symbols;
}

} // namespace

extern "C" t_symbol* gensym(const char* text)
{
    auto& symbols = Symbols();
    for (auto& entry : symbols)
    {
        if (entry.text == text)
        {
            return &entry.symbol;
        }
    }

    symbols.push_back(SymbolEntry{std::string(text), {}});
    auto& entry = symbols.back();
    entry.symbol.s_name = entry.text.data();
    entry.symbol.s_thing = nullptr;
    return &entry.symbol;
}

extern "C" t_symbol* symbol_unique(void)
{
    static int uniqueId = 0;
    return gensym(("unique_" + std::to_string(++uniqueId)).c_str());
}

extern "C" t_max_err atom_setlong(t_atom* atom, t_atom_long value)
{
    atom->a_type = A_LONG;
    atom->a_w.w_long = value;
    return MAX_ERR_NONE;
}

extern "C" t_max_err atom_setfloat(t_atom* atom, t_atom_float value)
{
    atom->a_type = A_FLOAT;
    atom->a_w.w_float = value;
    return MAX_ERR_NONE;
}

extern "C" t_max_err atom_setsym(t_atom* atom, t_symbol* value)
{
    atom->a_type = A_SYM;
    atom->a_w.w_sym = value;
    return MAX_ERR_NONE;
}

extern "C" t_atom_long atom_getlong(const t_atom* atom)
{
    return atom->a_w.w_long;
}

extern "C" t_atom_float atom_getfloat(const t_atom* atom)
{
    return atom->a_w.w_float;
}

extern "C" t_symbol* atom_getsym(const t_atom* atom)
{
    return atom->a_w.w_sym;
}

extern "C" t_symbol* object_classname(void*)
{
    return gensym("");
}

} // namespace c74::max

namespace c74::min
{

symbol::symbol(const atom& value)
    : s(c74::max::atom_getsym(&value))
{
}

} // namespace c74::min
