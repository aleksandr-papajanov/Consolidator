#include "Log.h"

#if CONSOLIDATOR_LOG_ENABLED

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace consolidator::core::logging
{

Logger& Logger::Instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger()
{
    namespace fs = std::filesystem;
    const auto logsDir = fs::current_path().parent_path() / "Logs";
    fs::create_directories(logsDir);

    const auto now = std::chrono::system_clock::now();
    const auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm {};
    localtime_s(&tm, &t);

    std::ostringstream name;
    name << "consolidator_"
         << std::put_time(&tm, "%Y%m%d_%H%M%S")
         << ".log";

    file_.open(logsDir / name.str(), std::ios::out | std::ios::app);
}

Logger::~Logger()
{
    if (file_.is_open())
        file_.close();
}

void Logger::Write(std::string_view category, std::string_view message)
{
    const auto now = std::chrono::system_clock::now();
    const auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm {};
    localtime_s(&tm, &t);

    std::ostringstream line;
    line << std::put_time(&tm, "%H:%M:%S")
         << " [" << category << "] "
         << message << "\n";

    const auto str = line.str();
    std::lock_guard lock(mutex_);
    file_.write(str.data(), static_cast<std::streamsize>(str.size()));
    file_.flush();
}

} // namespace consolidator::core::logging

#endif