#include "AtomPathCodec.h"

#include <string>

#include "Core/Domain/Ids/DspIds.h"
#include "BankIdCodec.h"

namespace consolidator::max
{
namespace
{

using namespace c74::min;

std::string Text(const atom& value)
{
    return static_cast<std::string>(value);
}

std::optional<int> Number(const atom& value)
{
    if (value.a_type != c74::max::A_LONG)
    {
        return std::nullopt;
    }

    return static_cast<int>(value);
}

std::optional<dsp::DeviceId> Device(const std::string& value)
{
    if (value == "main_input_gain")
    {
        return dsp::DeviceId::MainInputGain;
    }
    if (value == "main_output_gain")
    {
        return dsp::DeviceId::MainOutputGain;
    }
    if (value == "saturator")
    {
        return dsp::DeviceId::Saturator;
    }
    if (value == "compressor")
    {
        return dsp::DeviceId::Compressor;
    }
    if (value == "equalizer")
    {
        return dsp::DeviceId::Equalizer;
    }
    return std::nullopt;
}

std::optional<dsp::ParameterId> Parameter(const std::string& value)
{
    if (value == "gain")
    {
        return dsp::ParameterId::Gain;
    }
    if (value == "frequency")
    {
        return dsp::ParameterId::Frequency;
    }
    if (value == "q")
    {
        return dsp::ParameterId::Q;
    }
    if (value == "drive")
    {
        return dsp::ParameterId::Drive;
    }
    if (value == "mix")
    {
        return dsp::ParameterId::Mix;
    }
    if (value == "detector_amount")
    {
        return dsp::ParameterId::DetectorAmount;
    }
    if (value == "threshold")
    {
        return dsp::ParameterId::Threshold;
    }
    if (value == "ratio")
    {
        return dsp::ParameterId::Ratio;
    }
    if (value == "attack")
    {
        return dsp::ParameterId::Attack;
    }
    if (value == "release")
    {
        return dsp::ParameterId::Release;
    }
    return std::nullopt;
}

const char* DeviceName(dsp::DeviceId value)
{
    switch (value)
    {
    case dsp::DeviceId::MainInputGain:
        return "main_input_gain";
    case dsp::DeviceId::MainOutputGain:
        return "main_output_gain";
    case dsp::DeviceId::Saturator:
        return "saturator";
    case dsp::DeviceId::Compressor:
        return "compressor";
    case dsp::DeviceId::Equalizer:
        return "equalizer";
    }

    return "";
}

const char* ParameterName(dsp::ParameterId value)
{
    switch (value)
    {
    case dsp::ParameterId::Gain:
        return "gain";
    case dsp::ParameterId::Frequency:
        return "frequency";
    case dsp::ParameterId::Q:
        return "q";
    case dsp::ParameterId::Drive:
        return "drive";
    case dsp::ParameterId::Mix:
        return "mix";
    case dsp::ParameterId::DetectorAmount:
        return "detector_amount";
    case dsp::ParameterId::Threshold:
        return "threshold";
    case dsp::ParameterId::Ratio:
        return "ratio";
    case dsp::ParameterId::Attack:
        return "attack";
    case dsp::ParameterId::Release:
        return "release";
    }

    return "";
}

void EncodeNode(atoms& output, dsp::RouteNodeId node)
{
    const auto value = static_cast<int>(node);
    if (node == dsp::RouteNodeId::Detector)
    {
        output.emplace_back("detector");
    }
    else if (node >= dsp::RouteNodeId::Bank0 &&
             node <= dsp::RouteNodeId::Bank6)
    {
        output.emplace_back("bank");
        output.emplace_back(EncodeBankId(static_cast<dsp::BankId>(
            value - static_cast<int>(dsp::RouteNodeId::Bank0))));
    }
    else if (node >= dsp::RouteNodeId::Filter1 &&
             node <= dsp::RouteNodeId::Filter7)
    {
        output.emplace_back("filter");
        output.emplace_back(value - static_cast<int>(dsp::RouteNodeId::Filter1) + 1);
    }
}

bool IsMarker(const std::string& value)
{
    return value == "bypass" || value == "solo" || value == "listen";
}

void SetMarker(core::StatePath& path, const std::string& value)
{
    path.field = core::StateField::DspMarker;
    if (value == "bypass")
    {
        path.markerId = core::StateMarkerId::Bypass;
    }
    if (value == "solo")
    {
        path.markerId = core::StateMarkerId::Solo;
    }
    if (value == "listen")
    {
        path.markerId = core::StateMarkerId::Listen;
    }
}

} // namespace

std::optional<core::StatePath> AtomPathCodec::Decode(
    const atoms& input,
    std::size_t& position,
    std::size_t end,
    core::InstanceId instance) const
{
    if (position >= end || input[position].a_type != c74::max::A_SYM)
    {
        return std::nullopt;
    }

    const auto first = Text(input[position++]);
    core::StatePath path;
    path.instanceId = instance;

    if (first == "instance_id")
    {
        path.field = core::StateField::InstanceId;
        return path;
    }
    if (first == "label")
    {
        path.field = core::StateField::Label;
        return path;
    }
    if (first == "selected_bank")
    {
        path.field = core::StateField::SelectedBank;
        return path;
    }
    if (first == "mute")
    {
        path.field = core::StateField::Mute;
        return path;
    }
    if (first == "solo")
    {
        path.field = core::StateField::Solo;
        return path;
    }

    const auto parseBank = [&](core::StatePath& target) -> bool
    {
        if (position >= end)
        {
            return false;
        }
        const auto bank = Number(input[position++]);
        if (!bank || !DecodeBankId(*bank))
        {
            return false;
        }
        target.nodes[0] = static_cast<dsp::RouteNodeId>(
            static_cast<int>(dsp::RouteNodeId::Bank0) +
            static_cast<int>(*DecodeBankId(*bank)));
        target.depth = 1;
        return true;
    };

    const auto parseFilter = [&](core::StatePath& target, std::size_t nodeIndex) -> bool
    {
        if (position >= end || Text(input[position++]) != "filter" ||
            position >= end)
        {
            return false;
        }
        const auto filter = Number(input[position++]);
        const auto maximumFilter = target.nodes[0] == dsp::RouteNodeId::Detector
            ? 2
            : 7;
        if (!filter || *filter < 1 || *filter > maximumFilter)
        {
            return false;
        }
        target.nodes[nodeIndex] = static_cast<dsp::RouteNodeId>(
            static_cast<int>(dsp::RouteNodeId::Filter1) + *filter - 1);
        target.depth = nodeIndex + 1;
        return true;
    };

    if (first == "bank")
    {
        if (!parseBank(path))
        {
            return std::nullopt;
        }
        if (position == end)
        {
            path.field = core::StateField::BankId;
            return path;
        }
        if (Text(input[position]) == "group")
        {
            ++position;
            path.field = core::StateField::GroupId;
            return path;
        }
        return std::nullopt;
    }
    else
    {
        const auto device = Device(first);
        if (!device)
        {
            return std::nullopt;
        }
        path.deviceId = *device;
        if (position == end)
        {
            return path;
        }

        if (*device == dsp::DeviceId::Equalizer && Text(input[position]) == "bank")
        {
            ++position;
            if (!parseBank(path))
            {
                return std::nullopt;
            }
            if (position == end)
            {
                return path;
            }
            if (Text(input[position]) == "filter")
            {
                if (!parseFilter(path, 1))
                {
                    return std::nullopt;
                }
                if (position == end)
                {
                    return path;
                }
            }
            else if (!IsMarker(Text(input[position])) &&
                     !Parameter(Text(input[position])))
            {
                return std::nullopt;
            }
        }

        if (position < end && Text(input[position]) == "detector")
        {
            ++position;
            path.nodes[0] = dsp::RouteNodeId::Detector;
            path.depth = 1;
            if (position == end)
            {
                return path;
            }
            if (Text(input[position]) == "filter")
            {
                if (!parseFilter(path, 1))
                {
                    return std::nullopt;
                }
                if (position == end)
                {
                    return path;
                }
            }
            else if (!IsMarker(Text(input[position])) &&
                     !Parameter(Text(input[position])))
            {
                return std::nullopt;
            }
        }
    }

    if (position == end)
    {
        return path;
    }
    const auto name = Text(input[position++]);
    if (IsMarker(name))
    {
        SetMarker(path, name);
        return path;
    }

    const auto parameter = Parameter(name);
    if (!parameter)
    {
        return std::nullopt;
    }
    path.field = core::StateField::DspParameter;
    path.parameterId = *parameter;
    return path;
}

void AtomPathCodec::Encode(atoms& output, const core::StatePath& path) const
{
    if (path.field == core::StateField::InstanceId)
    {
        output.emplace_back("instance_id");
        return;
    }
    if (path.field == core::StateField::Label)
    {
        output.emplace_back("label");
        return;
    }
    if (path.field == core::StateField::SelectedBank)
    {
        output.emplace_back("selected_bank");
        return;
    }
    if (path.field == core::StateField::BankId && path.depth == 1)
    {
        output.emplace_back("bank");
        output.emplace_back(
            EncodeBankId(static_cast<dsp::BankId>(
                static_cast<int>(path.nodes[0]) -
                static_cast<int>(dsp::RouteNodeId::Bank0))));
        return;
    }
    if (path.field == core::StateField::GroupId && path.depth == 1)
    {
        output.emplace_back("bank");
        output.emplace_back(
            EncodeBankId(static_cast<dsp::BankId>(
                static_cast<int>(path.nodes[0]) -
                static_cast<int>(dsp::RouteNodeId::Bank0))));
        output.emplace_back("group");
        return;
    }
    if (path.field == core::StateField::Mute)
    {
        output.emplace_back("mute");
        return;
    }
    if (path.field == core::StateField::Solo && !path.deviceId)
    {
        output.emplace_back("solo");
        return;
    }
    if (!path.deviceId)
    {
        return;
    }

    output.emplace_back(DeviceName(*path.deviceId));
    for (std::size_t index = 0; index < path.depth; ++index)
    {
        EncodeNode(output, path.nodes[index]);
    }

    if (path.field == core::StateField::DspMarker && path.markerId)
    {
        output.emplace_back(
            *path.markerId == core::StateMarkerId::Bypass ? "bypass" :
            *path.markerId == core::StateMarkerId::Solo ? "solo" : "listen");
    }
    else if (path.field == core::StateField::DspParameter && path.parameterId)
    {
        output.emplace_back(ParameterName(*path.parameterId));
    }
}

} // namespace consolidator::max
