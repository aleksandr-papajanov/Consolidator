#include "Shared/Clients/TargetStateClient.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace consolidator::vst3::shared::clients
{

namespace
{
shared::clients::ParameterEntry ReadEntry(const std::vector<shared::protocol::ProtocolValue>& values, std::size_t offset)
{
    return { shared::protocol::Symbol(values.at(offset)), values.at(offset + 1),
        shared::protocol::OptionalNumber(values.at(offset + 2)), shared::protocol::OptionalNumber(values.at(offset + 3)),
        shared::protocol::OptionalNumber(values.at(offset + 4)), shared::protocol::OptionalNumber(values.at(offset + 5)) };
}
}

TargetStateClient::TargetStateClient(ProtocolClient& commands) : commands_(commands)
{
}

void TargetStateClient::Observe(UiTarget target)
{
    Invalidate();
    target_ = std::move(target);
    // All requests select the same bank; EQ is last to retain the main graph focus.
    for (const auto* context : { "gain_staging", "saturator", "compressor", "polish", "equalizer" })
    {
        const auto request = commands_.Request("observe_target", { std::to_string(target_.instance),
            static_cast<std::int64_t>(target_.bank), std::string(context) });
        if (request == 0)
        {
            throw std::runtime_error("Unable to request channel strip snapshot.");
        }
        snapshotRequests_.emplace(request, context);
    }
}

void TargetStateClient::Invalidate()
{
    ++generation_;
    snapshotRequests_.clear();
    acceptedContexts_.clear();
    stagedValues_.clear();
    ready_ = false;
    values_.clear();
}

bool TargetStateClient::HandleSnapshot(const shared::protocol::ProtocolFrame& frame)
{
    const auto& values = frame.values;
    if (values.size() < 7 || !snapshotRequests_.contains(shared::protocol::WireId(values[2])))
    {
        return false;
    }
    const auto& context = snapshotRequests_.at(shared::protocol::WireId(values[2]));
    if (acceptedContexts_.contains(context))
    {
        return false;
    }
    const auto count = shared::protocol::Integer(values[6]);
    if (count < 0 || static_cast<std::uint64_t>(count) > (values.size() - 7) / 6 ||
        values.size() != 7 + static_cast<std::size_t>(count) * 6 ||
        shared::protocol::WireId(values[3]) != target_.instance || shared::protocol::Integer(values[4]) != target_.bank ||
        shared::protocol::Symbol(values[5]) != context)
    {
        throw std::invalid_argument("Invalid target state snapshot.");
    }
    std::vector<shared::clients::ParameterEntry> entries;
    entries.reserve(static_cast<std::size_t>(count));
    std::unordered_set<std::string> paths;
    for (std::size_t offset = 7; offset < values.size(); offset += 6)
    {
        auto entry = ReadEntry(values, offset);
        if (!paths.insert(entry.path).second)
        {
            throw std::invalid_argument("Duplicate target state binding: " + entry.path);
        }
        entries.push_back(std::move(entry));
    }
    for (const auto& entry : stagedValues_)
    {
        if (paths.contains(entry.path))
        {
            throw std::invalid_argument("Duplicate channel strip binding: " + entry.path);
        }
    }
    stagedValues_.insert(stagedValues_.end(), std::make_move_iterator(entries.begin()),
        std::make_move_iterator(entries.end()));
    acceptedContexts_.insert(context);
    if (acceptedContexts_.size() != snapshotRequests_.size())
    {
        return false;
    }
    values_ = std::move(stagedValues_);
    ready_ = true;
    return true;
}

std::optional<shared::clients::ParameterEntry> TargetStateClient::HandleChanged(const shared::protocol::ProtocolFrame& frame)
{
    if (frame.values.size() != 8)
    {
        throw std::invalid_argument("Invalid state change frame.");
    }
    // Notifications include the "ready" status between value and range metadata.
    if (shared::protocol::Symbol(frame.values[3]) != "ready")
    {
        throw std::invalid_argument("Invalid state change status.");
    }
    shared::clients::ParameterEntry entry{ shared::protocol::Symbol(frame.values[1]), frame.values[2],
        shared::protocol::OptionalNumber(frame.values[4]), shared::protocol::OptionalNumber(frame.values[5]),
        shared::protocol::OptionalNumber(frame.values[6]), shared::protocol::OptionalNumber(frame.values[7]) };
    auto& destination = ready_ ? values_ : stagedValues_;
    const auto found = std::find_if(destination.begin(), destination.end(),
        [&entry](const auto& current) { return current.path == entry.path; });
    if (found == destination.end())
    {
        return {};
    }
    *found = entry;
    return ready_ ? std::optional<ParameterEntry>{ entry } : std::nullopt;
}

bool TargetStateClient::IsSnapshotRequest(std::uint64_t request) const
{
    return snapshotRequests_.contains(request);
}

bool TargetStateClient::Ready() const { return ready_; }
std::uint64_t TargetStateClient::Generation() const { return generation_; }
const UiTarget& TargetStateClient::Target() const { return target_; }
const std::vector<shared::clients::ParameterEntry>& TargetStateClient::Values() const { return values_; }

const shared::clients::ParameterEntry& TargetStateClient::Require(const std::string& path) const
{
    const auto found = std::find_if(values_.begin(), values_.end(),
        [&path](const auto& entry) { return entry.path == path; });
    if (!ready_ || found == values_.end())
    {
        throw std::invalid_argument("Missing target state binding: " + path);
    }
    return *found;
}

std::string TargetStateClient::WriteMode(const std::string& path, bool groupScope) const
{
    const auto& entry = Require(path);
    return groupScope && std::holds_alternative<double>(entry.value) && entry.minimum && entry.maximum
        ? "delta" : "copy";
}

} // namespace consolidator::vst3::shared::clients
