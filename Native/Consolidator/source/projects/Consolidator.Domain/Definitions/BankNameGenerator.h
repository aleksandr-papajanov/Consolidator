#pragma once

#include <array>
#include <string>

namespace consolidator::domain {

class BankNameGenerator final {
public:
    static std::string Generate(long bankId) {
        static constexpr std::array adjectives{
            "Neon", "Velvet", "Crystal", "Electric", "Silver", "Quiet", "Rapid", "Solar"
        };
        static constexpr std::array nouns{
            "Circuit", "Echo", "Pulse", "Signal", "Phase", "Vector", "Orbit", "Wave"
        };
        const auto index = static_cast<std::size_t>(bankId > 0 ? bankId - 1 : 0);
        auto result = std::string{ adjectives[index % adjectives.size()] } + " " +
            nouns[(index / adjectives.size()) % nouns.size()];
        const auto cycle = index / (adjectives.size() * nouns.size());
        if (cycle > 0) result += " " + std::to_string(cycle + 1);
        return result;
    }
};

} // namespace consolidator::domain
