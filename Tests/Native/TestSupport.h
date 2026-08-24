#pragma once

#include <iostream>
#include <string_view>

namespace consolidator::tests
{

inline bool Expect(
    bool condition,
    std::string_view message)
{
    if (condition)
    {
        return true;
    }

    std::cerr << message << '\n';
    return false;
}

bool RunAbiContractTests();
bool RunDspStateConsumerTests();

} // namespace consolidator::tests
