#include <Windows.h>
#include <cstdint>
#include <iostream>

struct SharedState
{
    std::int32_t revision;
    float gain;
    float frequency;
};

using CreateStateFn = SharedState * (*)();
using TestUpdateFn = void (*)();
using DestroyStateFn = void (*)();

int main()
{
    HMODULE library =
        LoadLibraryW(L"Consolidator.Managed.dll");

    if (!library)
    {
        std::cerr << "Failed to load DLL\n";
        return 1;
    }

    auto createState =
        reinterpret_cast<CreateStateFn>(
            GetProcAddress(
                library,
                "ConsolidatorCreateState"));

    auto testUpdate =
        reinterpret_cast<TestUpdateFn>(
            GetProcAddress(
                library,
                "ConsolidatorTestUpdate"));

    auto destroyState =
        reinterpret_cast<DestroyStateFn>(
            GetProcAddress(
                library,
                "ConsolidatorDestroyState"));

    if (!createState || !testUpdate || !destroyState)
    {
        std::cerr << "Failed to find exported functions\n";
        return 1;
    }

    SharedState* state = createState();

    std::cout << "Revision: " << state->revision << '\n';
    std::cout << "Gain: " << state->gain << '\n';
    std::cout << "Frequency: " << state->frequency << '\n';

    testUpdate();

    std::cout << "\nAfter update:\n";
    std::cout << "Revision: " << state->revision << '\n';
    std::cout << "Gain: " << state->gain << '\n';
    std::cout << "Frequency: " << state->frequency << '\n';

    destroyState();
    FreeLibrary(library);

    return 0;
}