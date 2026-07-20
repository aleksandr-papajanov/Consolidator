#pragma once

#include <compare>
#include <cstdint>

namespace consolidator::domain {

template <typename Tag>
struct Identifier {
    std::int64_t value = 0;

    constexpr auto operator<=>(const Identifier&) const = default;
    constexpr explicit operator bool() const noexcept { return value > 0; }
};

struct BankIdTag;
struct FilterIdTag;
struct ComponentIdTag;
struct RequestIdTag;
struct EventIdTag;
struct SessionIdTag;

using BankId = Identifier<BankIdTag>;
using FilterId = Identifier<FilterIdTag>;
using ComponentId = Identifier<ComponentIdTag>;
using RequestId = Identifier<RequestIdTag>;
using EventId = Identifier<EventIdTag>;
using SessionId = Identifier<SessionIdTag>;

using StoreRevision = std::uint64_t;
using ProtocolVersion = std::int64_t;

} // namespace consolidator::domain
