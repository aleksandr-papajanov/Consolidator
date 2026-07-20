#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace consolidator::messaging {

using AtomValue = std::variant<std::int64_t, double, bool, std::string>;
using AtomList = std::vector<AtomValue>;

enum class MessageCategory {
    Command,
    Event,
    Snapshot
};

struct ProtocolError {
    std::string code;
    std::size_t fieldIndex = 0;
    std::string expected;
    std::string actual;
};

} // namespace consolidator::messaging
