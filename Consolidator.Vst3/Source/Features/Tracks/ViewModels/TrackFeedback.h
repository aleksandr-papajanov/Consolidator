#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>

namespace consolidator::vst3::features::tracks::viewmodels
{

class TrackFeedback final
{
public:
    [[nodiscard]] bool Bypass(std::uint64_t instance, const std::string& item, bool committed) const;
    void SetBypass(std::uint64_t instance, std::string item, bool value);
    void ConfirmBypass(std::uint64_t instance, const std::string& item, bool committed);
    void Clear();

private:
    std::map<std::pair<std::uint64_t, std::string>, bool> bypass_;
};

} // namespace consolidator::vst3::features::tracks::viewmodels
