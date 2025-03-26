#include "RetroGBm/Pch.h"
#include "RetroGBm/Logger.h"

#define FMT_UNICODE 0
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

std::shared_ptr<spdlog::logger> Logger::sm_ConsoleLogger = nullptr;
std::shared_ptr<spdlog::logger> Logger::sm_FileLogger = nullptr;

namespace
{
	spdlog::level::level_enum GetSpdLogLevel(LogLevel level)
	{
		switch (level)
		{
			case LOG_INFO:
				return spdlog::level::info;
			case LOG_WARNING:
				return spdlog::level::warn;
			case LOG_ERROR:
				return spdlog::level::err;
			case LOG_CRITICAL:
				return spdlog::level::critical;
			default:
				return spdlog::level::info;
		}
	}
}

void Logger::Initialise(LogLevel level)
{
	spdlog::set_pattern("%^[%H:%M:%S.%e] [thread %t] %l: %v%$");

	if (!spdlog::get("console"))
	{
		sm_ConsoleLogger = spdlog::stdout_color_mt("console");
		sm_ConsoleLogger->set_level(GetSpdLogLevel(level));
	}

	if (!spdlog::get("file"))
	{
		sm_FileLogger = spdlog::basic_logger_mt("file", "errorlog.txt");
		sm_FileLogger->set_level(spdlog::level::err);
	}
}

void Logger::Info(const std::string& message)
{
	if (sm_ConsoleLogger != nullptr)
	{
		sm_ConsoleLogger->info(message);
	}
	else
	{
		spdlog::info(message);
	}
}

void Logger::Warning(const std::string& message)
{
	if (sm_ConsoleLogger != nullptr)
	{
		sm_ConsoleLogger->warn(message);
	}
	else
	{
		spdlog::warn(message);
	}
}

void Logger::Error(const std::string& message)
{
	if (sm_ConsoleLogger != nullptr)
	{
		sm_ConsoleLogger->error(message);
		sm_ConsoleLogger->error(message);
	}
	else
	{
		spdlog::error(message);
	}

	if (sm_FileLogger != nullptr)
	{
		sm_FileLogger->error(message);
	}
}

void Logger::Critical(const std::string& message)
{
	sm_ConsoleLogger->critical(message);
	sm_FileLogger->critical(message);
}

void Logger::SetConsoleLogLevel(LogLevel level)
{
	sm_ConsoleLogger->set_level(GetSpdLogLevel(level));
}

void Logger::SetFileLogLevel(LogLevel level)
{
	sm_FileLogger->set_level(GetSpdLogLevel(level));
}