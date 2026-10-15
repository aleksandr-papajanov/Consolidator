#include "Shared/Clients/RegistryClient.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace consolidator::vst3::shared::clients
{

namespace
{

void RequireSize(const std::vector<shared::protocol::ProtocolValue>& values, std::size_t count)
{
    if (values.size() != count)
    {
        throw std::invalid_argument("Invalid registry frame length.");
    }
}

std::size_t Count(const shared::protocol::ProtocolValue& value)
{
    const auto count = shared::protocol::Integer(value);
    if (count < 0)
    {
        throw std::invalid_argument("Negative registry count.");
    }
    return static_cast<std::size_t>(count);
}

int BankIndex(const shared::protocol::ProtocolValue& value)
{
    const auto index = shared::protocol::Integer(value);
    if (index < 0 || index > 6)
    {
        throw std::invalid_argument("Invalid registry bank index.");
    }
    return static_cast<int>(index);
}

std::optional<std::uint32_t> Group(const shared::protocol::ProtocolValue& value)
{
    if (const auto symbol = std::get_if<std::string>(&value); symbol != nullptr && *symbol == "none")
    {
        return {};
    }
    const auto group = shared::protocol::Integer(value);
    if (group < 0 || group > std::numeric_limits<std::uint32_t>::max())
    {
        throw std::invalid_argument("Invalid registry group ID.");
    }
    return static_cast<std::uint32_t>(group);
}

RegistryInstance& FindInstance(std::vector<RegistryInstance>& instances, std::uint64_t id)
{
    const auto found = std::find_if(instances.begin(), instances.end(), [id](const auto& item) { return item.id == id; });
    if (found == instances.end())
    {
        throw std::invalid_argument("Registry references an unknown instance.");
    }
    return *found;
}

RegistryBank& RequireBank(std::vector<RegistryInstance>& instances, std::uint64_t id, int bank)
{
    auto& instance = FindInstance(instances, id);
    const auto found = std::find_if(instance.banks.begin(), instance.banks.end(),
        [bank](const auto& item) { return item.index == bank; });
    if (found == instance.banks.end())
    {
        throw std::invalid_argument("Registry references an unknown bank.");
    }
    return *found;
}

RegistryProcessor& FindProcessor(RegistryInstance& instance, const std::string& id)
{
    const auto found = std::find_if(instance.processors.begin(), instance.processors.end(),
        [&id](const auto& item) { return item.id == id; });
    if (found == instance.processors.end())
    {
        throw std::invalid_argument("Registry references an unknown processor.");
    }
    return *found;
}

RegistryInstance DecodeInstance(const std::vector<shared::protocol::ProtocolValue>& values)
{
    if (values.size() < 8)
    {
        throw std::invalid_argument("Truncated registry instance.");
    }
    return { shared::protocol::WireId(values[3]), shared::protocol::Symbol(values[4]), shared::protocol::Boolean(values[5]), shared::protocol::Boolean(values[6]), shared::protocol::Boolean(values[7]), {}, {} };
}

RegistryBank DecodeBank(const std::vector<shared::protocol::ProtocolValue>& values, std::size_t offset)
{
    if (offset > values.size() || values.size() - offset < 4)
    {
        throw std::invalid_argument("Truncated registry bank.");
    }
    return { BankIndex(values[offset]), Group(values[offset + 1]), shared::protocol::Boolean(values[offset + 2]), shared::protocol::Boolean(values[offset + 3]) };
}

bool IsDelta(const std::string& selector)
{
    return selector == "registry_instance_added" || selector == "registry_instance_removed" ||
        selector == "registry_label_changed" || selector == "registry_instance_mute_changed" ||
        selector == "registry_instance_solo_changed" || selector == "registry_instance_bypass_changed" ||
        selector == "registry_processor_changed" || selector == "registry_bank_group_changed" ||
        selector == "registry_bank_effect_changed" || selector == "registry_bank_bypass_changed";
}

} // namespace

void RegistryClient::StartSnapshot(std::uint64_t request)
{
    snapshot_ = Snapshot{ request };
}

void RegistryClient::Invalidate()
{
    snapshot_.reset();
    ready_ = false;
}

void RegistryClient::CancelSnapshot(std::uint64_t request)
{
    if (snapshot_ && snapshot_->request == request)
    {
        snapshot_.reset();
    }
}

bool RegistryClient::Ready() const { return ready_; }
bool RegistryClient::FetchPending() const { return snapshot_.has_value(); }
const std::vector<RegistryInstance>& RegistryClient::Instances() const { return instances_; }

const RegistryInstance* RegistryClient::Find(std::uint64_t instance) const
{
    const auto found = std::find_if(instances_.begin(), instances_.end(),
        [instance](const auto& item) { return item.id == instance; });
    return found == instances_.end() ? nullptr : &*found;
}

const RegistryBank* RegistryClient::FindBank(std::uint64_t instance, int bank) const
{
    const auto row = Find(instance);
    if (row == nullptr)
    {
        return nullptr;
    }
    const auto found = std::find_if(row->banks.begin(), row->banks.end(),
        [bank](const auto& item) { return item.index == bank; });
    return found == row->banks.end() ? nullptr : &*found;
}

std::optional<std::uint32_t> RegistryClient::NextGroup() const
{
    std::set<std::uint32_t> used{ 0 };
    for (const auto& instance : instances_)
    {
        for (const auto& bank : instance.banks)
        {
            if (bank.group)
            {
                used.insert(*bank.group);
            }
        }
    }
    // Same sixteen-group palette/creation limit as Max TrackActions.
    for (std::uint32_t next = 1; next < 16; ++next)
    {
        if (!used.contains(next))
        {
            return next;
        }
    }
    return {};
}

RegistryUpdate RegistryClient::Handle(const shared::protocol::ProtocolFrame& frame)
{
    const auto& values = frame.values;
    if (IsDelta(frame.selector))
    {
        if (values.size() < 4)
        {
            throw std::invalid_argument("Truncated registry delta.");
        }
        const auto previous = shared::protocol::Integer(values[1]);
        const auto revision = shared::protocol::Integer(values[2]);
        if (previous < 0 || revision <= previous)
        {
            throw std::invalid_argument("Invalid registry revision.");
        }
        requiredRevision_ = std::max(requiredRevision_, revision);
        if (!ready_ || revision_ != previous)
        {
            return RegistryUpdate::Resynchronize;
        }
        ApplyDelta(frame);
        revision_ = revision;
        return RegistryUpdate::Changed;
    }
    if (frame.selector == "registry_processor_markers_changed")
    {
        if (ready_)
        {
            ApplyMarkers(frame);
            return RegistryUpdate::PresentationChanged;
        }
        return RegistryUpdate::Ignored;
    }
    if (!frame.selector.starts_with("registry_") || values.size() < 3 ||
        !snapshot_ || shared::protocol::WireId(values[2]) != snapshot_->request)
    {
        return RegistryUpdate::Ignored;
    }
    auto& snapshot = *snapshot_;
    if (frame.selector == "registry_begin")
    {
        RequireSize(values, 6);
        if (snapshot.began)
        {
            throw std::invalid_argument("Duplicate registry snapshot begin.");
        }
        snapshot.revision = shared::protocol::Integer(values[3]);
        snapshot.expectedInstances = Count(values[4]);
        snapshot.expectedGroups = Count(values[5]);
        snapshot.began = true;
        return RegistryUpdate::Ignored;
    }
    if (!snapshot.began)
    {
        throw std::invalid_argument("Registry snapshot entry arrived before its header.");
    }
    if (frame.selector == "registry_instance")
    {
        RequireSize(values, 8);
        auto instance = DecodeInstance(values);
        if (std::any_of(snapshot.instances.begin(), snapshot.instances.end(),
                [&instance](const auto& item) { return item.id == instance.id; }))
        {
            throw std::invalid_argument("Duplicate registry instance.");
        }
        snapshot.instances.push_back(std::move(instance));
    }
    else if (frame.selector == "registry_processor")
    {
        RequireSize(values, 8);
        auto& instance = FindInstance(snapshot.instances, shared::protocol::WireId(values[3]));
        instance.processors.push_back({ shared::protocol::Symbol(values[4]), shared::protocol::Boolean(values[5]), shared::protocol::Boolean(values[6]), shared::protocol::Boolean(values[7]) });
    }
    else if (frame.selector == "registry_bank")
    {
        RequireSize(values, 8);
        FindInstance(snapshot.instances, shared::protocol::WireId(values[3])).banks.push_back(DecodeBank(values, 4));
    }
    else if (frame.selector == "registry_group")
    {
        RequireSize(values, 4);
        if (!Group(values[3]))
        {
            throw std::invalid_argument("A registry group must have an ID.");
        }
        ++snapshot.groupsReceived;
    }
    else if (frame.selector == "registry_member")
    {
        RequireSize(values, 6);
        const auto group = Group(values[3]);
        const auto& bank = RequireBank(snapshot.instances, shared::protocol::WireId(values[4]), BankIndex(values[5]));
        if (!group || bank.group != group)
        {
            throw std::invalid_argument("Registry group membership disagrees with its bank.");
        }
    }
    else if (frame.selector == "registry_done")
    {
        RequireSize(values, 4);
        if (shared::protocol::Integer(values[3]) != snapshot.revision || snapshot.instances.size() != snapshot.expectedInstances ||
            snapshot.groupsReceived != snapshot.expectedGroups)
        {
            throw std::invalid_argument("Incomplete registry snapshot.");
        }
        for (const auto& instance : snapshot.instances)
        {
            std::set<int> indexes;
            for (const auto& bank : instance.banks)
            {
                indexes.insert(bank.index);
            }
            if (instance.banks.size() != 7 || indexes.size() != 7)
            {
                throw std::invalid_argument("A registry instance requires all seven distinct banks.");
            }
        }
        if (snapshot.revision < std::max(requiredRevision_, revision_))
        {
            snapshot_.reset();
            return RegistryUpdate::Resynchronize;
        }
        instances_ = std::move(snapshot.instances);
        revision_ = snapshot.revision;
        ready_ = true;
        snapshot_.reset();
        return RegistryUpdate::Changed;
    }
    return RegistryUpdate::Ignored;
}

void RegistryClient::ApplyDelta(const shared::protocol::ProtocolFrame& frame)
{
    const auto& values = frame.values;
    const auto id = shared::protocol::WireId(values[3]);
    if (frame.selector == "registry_instance_added")
    {
        auto instance = DecodeInstance(values);
        if (values.size() < 10)
        {
            throw std::invalid_argument("Truncated added-instance payload.");
        }
        const auto processors = Count(values[8]);
        if (processors > (values.size() - 10) / 3)
        {
            throw std::invalid_argument("Invalid added-instance processor count.");
        }
        std::size_t offset = 9;
        for (std::size_t index = 0; index < processors; ++index, offset += 3)
        {
            instance.processors.push_back({ shared::protocol::Symbol(values[offset]), shared::protocol::Boolean(values[offset + 1]), false,
                shared::protocol::Boolean(values[offset + 2]) });
        }
        const auto banks = Count(values[offset++]);
        if (banks != 7 || banks > (values.size() - offset) / 4 || values.size() != offset + banks * 4 || Find(id))
        {
            throw std::invalid_argument("Invalid added-instance banks or duplicate ID.");
        }
        std::set<int> indexes;
        for (std::size_t index = 0; index < banks; ++index, offset += 4)
        {
            auto bank = DecodeBank(values, offset);
            if (!indexes.insert(bank.index).second)
            {
                throw std::invalid_argument("Duplicate added-instance bank.");
            }
            instance.banks.push_back(bank);
        }
        instances_.push_back(std::move(instance));
        return;
    }
    if (frame.selector == "registry_instance_removed")
    {
        RequireSize(values, 4);
        FindInstance(instances_, id);
        std::erase_if(instances_, [id](const auto& instance) { return instance.id == id; });
        return;
    }
    auto& instance = FindInstance(instances_, id);
    if (frame.selector == "registry_label_changed")
    {
        RequireSize(values, 5);
        instance.label = shared::protocol::Symbol(values[4]);
    }
    else if (frame.selector == "registry_instance_mute_changed" || frame.selector == "registry_instance_solo_changed" ||
        frame.selector == "registry_instance_bypass_changed")
    {
        RequireSize(values, 5);
        auto& flag = frame.selector == "registry_instance_mute_changed" ? instance.mute :
            frame.selector == "registry_instance_solo_changed" ? instance.solo : instance.bypass;
        flag = shared::protocol::Boolean(values[4]);
    }
    else if (frame.selector == "registry_processor_changed")
    {
        RequireSize(values, 7);
        auto& processor = FindProcessor(instance, shared::protocol::Symbol(values[4]));
        processor.active = shared::protocol::Boolean(values[5]);
        processor.bypass = shared::protocol::Boolean(values[6]);
    }
    else
    {
        RequireSize(values, 6);
        auto& bank = RequireBank(instances_, id, BankIndex(values[4]));
        if (frame.selector == "registry_bank_group_changed")
        {
            bank.group = Group(values[5]);
        }
        else if (frame.selector == "registry_bank_effect_changed")
        {
            bank.active = shared::protocol::Boolean(values[5]);
        }
        else
        {
            bank.bypass = shared::protocol::Boolean(values[5]);
        }
    }
}

void RegistryClient::ApplyMarkers(const shared::protocol::ProtocolFrame& frame)
{
    const auto& values = frame.values;
    if (values.size() < 2)
    {
        throw std::invalid_argument("Truncated processor markers.");
    }
    const auto instances = Count(values[1]);
    std::size_t offset = 2;
    for (std::size_t index = 0; index < instances; ++index)
    {
        if (offset > values.size() || values.size() - offset < 2)
        {
            throw std::invalid_argument("Truncated marker instance.");
        }
        const auto id = shared::protocol::WireId(values[offset++]);
        const auto processors = Count(values[offset++]);
        if (processors > (values.size() - offset) / 2)
        {
            throw std::invalid_argument("Truncated marker processors.");
        }
        for (std::size_t processor = 0; processor < processors; ++processor, offset += 2)
        {
            const auto& name = shared::protocol::Symbol(values[offset]);
            const auto active = shared::protocol::Boolean(values[offset + 1]);
            if (Find(id))
            {
                FindProcessor(FindInstance(instances_, id), name).marker = active;
            }
        }
    }
    RequireSize(values, offset);
}

} // namespace consolidator::vst3::shared::clients
