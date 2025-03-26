#pragma once

#include <string>
#include <memory>

namespace spdlog
{
	class logger;
}

enum LogLevel
{
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	LOG_CRITICAL
};

class Logger
{
public:
	void static Initialise(LogLevel level);

	static void Info(const std::string& message);
	static void Warning(const std::string& message);
	static void Error(const std::string& message);
	static void Critical(const std::string& message);

	static void SetConsoleLogLevel(LogLevel level);
	static void SetFileLogLevel(LogLevel level);

private:
	static std::shared_ptr<spdlog::logger> sm_ConsoleLogger;
	static std::shared_ptr<spdlog::logger> sm_FileLogger;
};
