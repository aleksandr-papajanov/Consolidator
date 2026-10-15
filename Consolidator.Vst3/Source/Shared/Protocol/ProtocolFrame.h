#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <optional>
#include <variant>
#include <vector>

#include "Transport/ManagedInterop.h"

namespace consolidator::vst3::shared::protocol
{

using ProtocolValue = std::variant<std::int64_t, double, std::string>;

struct ProtocolFrame
{
    std::string selector;
    std::vector<ProtocolValue> values;
};

struct StateWriteEntry
{
    std::string path;
    ProtocolValue value;
    std::string mode;
};

[[nodiscard]] std::vector<::consolidator::transport::NativeAtom> EncodeAtoms(
    const std::vector<ProtocolValue>& values);
[[nodiscard]] ProtocolFrame CopyFrame(
    const char* selector, const ::consolidator::transport::NativeAtom* atoms, std::size_t count);
[[nodiscard]] double Number(const ProtocolValue& value);
[[nodiscard]] const std::string& Symbol(const ProtocolValue& value);
[[nodiscard]] std::uint64_t WireId(const ProtocolValue& value);
[[nodiscard]] std::int64_t Integer(const ProtocolValue& value);
[[nodiscard]] bool Boolean(const ProtocolValue& value);
[[nodiscard]] std::optional<double> OptionalNumber(const ProtocolValue& value);
[[nodiscard]] std::vector<ProtocolValue> EncodePath(const std::string& path);
[[nodiscard]] std::vector<ProtocolValue> EncodeStateWrite(
    const std::vector<StateWriteEntry>& entries, const std::string& scope, const std::string& transaction);

} // namespace consolidator::vst3::shared::protocol
