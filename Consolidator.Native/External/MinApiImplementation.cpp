#include "c74_min.h"

namespace c74::min
{

template c74::min::attribute<
    c74::min::symbol,
    c74::min::threadsafe::undefined,
    c74::min::limit::none,
    c74::min::allow_repetitions::yes>::attribute(
    c74::min::object_base*,
    std::string,
    c74::min::symbol,
    c74::min::getter,
    c74::min::setter);

} // namespace c74::min
