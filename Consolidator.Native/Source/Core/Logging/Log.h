#pragma once

#ifdef CONSOLIDATOR_DEV
#define CONSOLIDATOR_LOG_ENABLED 1
#else
#define CONSOLIDATOR_LOG_ENABLED 0
#endif

#if CONSOLIDATOR_LOG_ENABLED

#include <fstream>
#include <mutex>
#include <string_view>

namespace consolidator::core::logging
{

class Logger
{
public:
    static Logger& Instance();
    void Write(std::string_view category, std::string_view message);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();
    ~Logger();
    std::ofstream file_;
    std::mutex mutex_;
};

} // namespace consolidator::core::logging

#define CONSOLIDATOR_LOG(category, message) \
    consolidator::core::logging::Logger::Instance().Write(category, message)

#else

#define CONSOLIDATOR_LOG(category, message) ((void)0)

#endif