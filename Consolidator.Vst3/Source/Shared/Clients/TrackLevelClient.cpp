#include "Shared/Clients/TrackLevelClient.h"

#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace consolidator::vst3::shared::clients
{

bool TrackLevelClient::Handle(const shared::protocol::ProtocolFrame& frame)
{
    if (frame.selector != "track_levels")
    {
        return false;
    }
    const auto& values = frame.values;
    if (values.size() < 3)
    {
        throw std::invalid_argument("Truncated track levels.");
    }
    const auto revision = shared::protocol::Integer(values[1]);
    if (revision <= revision_)
    {
        return false;
    }
    const auto count = shared::protocol::Integer(values[2]);
    if (count < 0 || static_cast<std::uint64_t>(count) > (values.size() - 3) / 2 ||
        values.size() != 3 + static_cast<std::size_t>(count) * 2)
    {
        throw std::invalid_argument("Invalid track level count.");
    }
    std::vector<std::pair<std::uint64_t, double>> entries;
    entries.reserve(static_cast<std::size_t>(count));
    for (std::size_t offset = 3; offset < values.size(); offset += 2)
    {
        const auto id = shared::protocol::Integer(values[offset]);
        const auto level = shared::protocol::Number(values[offset + 1]);
        if (id <= 0 || !std::isfinite(level))
        {
            throw std::invalid_argument("Invalid track level value.");
        }
        entries.emplace_back(static_cast<std::uint64_t>(id), level);
    }
    for (const auto& entry : entries)
    {
        levels_[entry.first] = entry.second;
    }
    revision_ = revision;
    return true;
}

double TrackLevelClient::Level(std::uint64_t instance) const
{
    const auto found = levels_.find(instance);
    return found != levels_.end() ? found->second : -120;
}

} // namespace consolidator::vst3::shared::clients
