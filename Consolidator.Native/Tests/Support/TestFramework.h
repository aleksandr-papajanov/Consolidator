#pragma once

#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace consolidator::test
{

struct TestCase
{
    std::string_view name;
    void (*body)();
};

inline std::vector<TestCase>& Registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

class Registration
{
public:
    Registration(std::string_view name, void (*body)())
    {
        Registry().push_back({name, body});
    }
};

[[noreturn]] inline void Fail(
    std::string_view expression,
    std::string_view file,
    int line)
{
    std::ostringstream message;
    message << file << ':' << line << ": expectation failed: " << expression;
    throw std::runtime_error(message.str());
}

template <typename Actual, typename Expected>
void ExpectEqual(
    const Actual& actual,
    const Expected& expected,
    std::string_view expression,
    std::string_view file,
    int line)
{
    if (!(actual == expected))
    {
        std::ostringstream details;
        details << expression;
        if constexpr (requires { details << actual; details << expected; })
        {
            details << " (actual: " << actual << ", expected: " << expected << ')';
        }
        Fail(details.str(), file, line);
    }
}

inline void ExpectNear(
    double actual,
    double expected,
    double tolerance,
    std::string_view expression,
    std::string_view file,
    int line)
{
    if (std::abs(actual - expected) > tolerance)
    {
        std::ostringstream details;
        details << expression << " (actual: " << actual
                << ", expected: " << expected
                << ", tolerance: " << tolerance << ')';
        Fail(details.str(), file, line);
    }
}

inline int RunAllTests()
{
    std::size_t failures = 0;
    for (const auto& test : Registry())
    {
        try
        {
            test.body();
            std::cout << "[PASS] " << test.name << '\n';
        }
        catch (const std::exception& error)
        {
            ++failures;
            std::cerr << "[FAIL] " << test.name << "\n  " << error.what() << '\n';
        }
        catch (...)
        {
            ++failures;
            std::cerr << "[FAIL] " << test.name << "\n  unknown exception\n";
        }
    }

    std::cout << Registry().size() - failures << '/' << Registry().size()
              << " tests passed\n";
    return failures == 0 ? 0 : 1;
}

} // namespace consolidator::test

#define CONSOLIDATOR_TEST_JOIN_IMPL(left, right) left##right
#define CONSOLIDATOR_TEST_JOIN(left, right) CONSOLIDATOR_TEST_JOIN_IMPL(left, right)
#define TEST_CASE(name) \
    static void CONSOLIDATOR_TEST_JOIN(TestBody_, __LINE__)(); \
    static const ::consolidator::test::Registration \
        CONSOLIDATOR_TEST_JOIN(TestRegistration_, __LINE__){ \
            name, &CONSOLIDATOR_TEST_JOIN(TestBody_, __LINE__)}; \
    static void CONSOLIDATOR_TEST_JOIN(TestBody_, __LINE__)()

#define EXPECT_TRUE(expression) \
    do { if (!(expression)) ::consolidator::test::Fail( \
        #expression, __FILE__, __LINE__); } while (false)
#define EXPECT_FALSE(expression) EXPECT_TRUE(!(expression))
#define EXPECT_EQ(actual, expected) \
    ::consolidator::test::ExpectEqual( \
        (actual), (expected), #actual " == " #expected, __FILE__, __LINE__)
#define EXPECT_NEAR(actual, expected, tolerance) \
    ::consolidator::test::ExpectNear( \
        static_cast<double>(actual), static_cast<double>(expected), \
        static_cast<double>(tolerance), #actual " ~= " #expected, __FILE__, __LINE__)

#define TEST_MAIN() \
    int main() { return ::consolidator::test::RunAllTests(); }
