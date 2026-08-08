#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "Dsp/Parameters/DspIds.h"

namespace consolidator::dsp
{

enum class RouteNodeId : std::uint8_t
{
    Detector,
    Bank0,
    Bank1,
    Bank2,
    Bank3,
    Bank4,
    Bank5,
    Bank6,
    Filter1,
    Filter2,
    Filter3,
    Filter4,
    Filter5,
    Filter6,
    Filter7
};

class ParameterRoute
{
public:
    static constexpr std::size_t kMaximumDepth = 3;

    constexpr ParameterRoute(
        DeviceId deviceId,
        ParameterId parameterId) noexcept
        : deviceId_(deviceId)
        , parameterId_(parameterId)
    {
    }

    template <typename... NodeIds>
    constexpr ParameterRoute(
        DeviceId deviceId,
        ParameterId parameterId,
        NodeIds... nodeIds) noexcept
        : deviceId_(deviceId)
        , parameterId_(parameterId)
        , nodes_{static_cast<RouteNodeId>(nodeIds)...}
        , depth_(sizeof...(nodeIds))
    {
        static_assert(sizeof...(nodeIds) <= kMaximumDepth);
    }

    [[nodiscard]] constexpr DeviceId GetDeviceId() const noexcept
    {
        return deviceId_;
    }

    [[nodiscard]] constexpr ParameterId GetParameterId() const noexcept
    {
        return parameterId_;
    }

    [[nodiscard]] constexpr std::size_t GetDepth() const noexcept
    {
        return depth_;
    }

    [[nodiscard]] constexpr RouteNodeId GetNode(std::size_t depth) const noexcept
    {
        return nodes_[depth];
    }

private:
    DeviceId deviceId_;
    ParameterId parameterId_;
    std::array<RouteNodeId, kMaximumDepth> nodes_{};
    std::size_t depth_ = 0;
};

} // namespace consolidator::dsp
