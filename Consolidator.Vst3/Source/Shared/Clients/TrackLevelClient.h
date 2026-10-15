#pragma once

#include <cstdint>
#include <map>

#include "Shared/Protocol/ProtocolFrame.h"

namespace consolidator::vst3::shared::clients
{

class TrackLevelClient final
{
public:
    [[nodiscard]] bool Handle(const shared::protocol::ProtocolFrame& frame);
    [[nodiscard]] double Level(std::uint64_t instance) const;

private:
    std::int64_t revision_{};
    std::map<std::uint64_t, double> levels_;
};

} // namespace consolidator::vst3::shared::clients
