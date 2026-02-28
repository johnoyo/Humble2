#pragma once

#include "Humble2API.h"
#include "Utilities\Collections\BitFlags.h"

#define FMT_UNICODE 0
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

// Ignore all warnings raised inside those headers
#pragma warning(push, 0)
#include <spdlog\spdlog.h>
#include <spdlog\fmt\ostr.h>
#include <spdlog\sinks\stdout_color_sinks.h>
#include <spdlog\sinks\rotating_file_sink.h>
#include <spdlog\async.h>
#include <spdlog\fmt\std.h>
#pragma warning(pop)

#include <memory>

namespace HBL2
{
	enum class LogContexts
	{
		NONE = 1,
		TERMINAL = 2,
		FILE = 8,
	};

	class HBL2_API Log
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void SetOutputs(BitFlags<LogContexts> outputs);

		static std::shared_ptr<spdlog::logger>& GetCoreLogger();
		static std::shared_ptr<spdlog::logger>& GetClientLogger();

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;

		static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> s_ConsoleSink;
		static std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> s_FileSink;
	};
}

#ifdef DEBUG
	#define HBL2_CORE_FATAL(...) SPDLOG_LOGGER_CRITICAL(::HBL2::Log::GetCoreLogger().get(), __VA_ARGS__)
	#define HBL2_CORE_ERROR(...) SPDLOG_LOGGER_ERROR(::HBL2::Log::GetCoreLogger().get(), __VA_ARGS__)
	#define HBL2_CORE_WARN(...)  SPDLOG_LOGGER_WARN(::HBL2::Log::GetCoreLogger().get(), __VA_ARGS__)
	#define HBL2_CORE_INFO(...)  SPDLOG_LOGGER_INFO(::HBL2::Log::GetCoreLogger().get(), __VA_ARGS__)
	#define HBL2_CORE_TRACE(...) SPDLOG_LOGGER_TRACE(::HBL2::Log::GetCoreLogger().get(), __VA_ARGS__)

	#define HBL2_FATAL(...) SPDLOG_LOGGER_CRITICAL(::HBL2::Log::GetClientLogger().get(), __VA_ARGS__)
	#define HBL2_ERROR(...) SPDLOG_LOGGER_ERROR(::HBL2::Log::GetClientLogger().get(), __VA_ARGS__)
	#define HBL2_WARN(...)  SPDLOG_LOGGER_WARN(::HBL2::Log::GetClientLogger().get(), __VA_ARGS__)
	#define HBL2_INFO(...)  SPDLOG_LOGGER_INFO(::HBL2::Log::GetClientLogger().get(), __VA_ARGS__)
	#define HBL2_TRACE(...) SPDLOG_LOGGER_TRACE(::HBL2::Log::GetClientLogger().get(), __VA_ARGS__)
#elif RELEASE
	#define HBL2_CORE_FATAL(...) SPDLOG_LOGGER_CRITICAL(::HBL2::Log::GetCoreLogger().get(), __VA_ARGS__)
	#define HBL2_CORE_ERROR(...) SPDLOG_LOGGER_ERROR(::HBL2::Log::GetCoreLogger().get(), __VA_ARGS__)
	#define HBL2_CORE_WARN(...)  SPDLOG_LOGGER_WARN(::HBL2::Log::GetCoreLogger().get(), __VA_ARGS__)
	#define HBL2_CORE_INFO(...)  SPDLOG_LOGGER_INFO(::HBL2::Log::GetCoreLogger().get(), __VA_ARGS__)
	#define HBL2_CORE_TRACE(...) SPDLOG_LOGGER_TRACE(::HBL2::Log::GetCoreLogger().get(), __VA_ARGS__)

	#define HBL2_FATAL(...) SPDLOG_LOGGER_CRITICAL(::HBL2::Log::GetClientLogger().get(), __VA_ARGS__)
	#define HBL2_ERROR(...) SPDLOG_LOGGER_ERROR(::HBL2::Log::GetClientLogger().get(), __VA_ARGS__)
	#define HBL2_WARN(...)  SPDLOG_LOGGER_WARN(::HBL2::Log::GetClientLogger().get(), __VA_ARGS__)
	#define HBL2_INFO(...)  SPDLOG_LOGGER_INFO(::HBL2::Log::GetClientLogger().get(), __VA_ARGS__)
	#define HBL2_TRACE(...) SPDLOG_LOGGER_TRACE(::HBL2::Log::GetClientLogger().get(), __VA_ARGS__)
#else
	#define HBL2_CORE_FATAL(...)
	#define HBL2_CORE_ERROR(...)
	#define HBL2_CORE_WARN(...) 
	#define HBL2_CORE_INFO(...) 
	#define HBL2_CORE_TRACE(...)

	#define HBL2_FATAL(...)
	#define HBL2_ERROR(...)
	#define HBL2_WARN(...) 
	#define HBL2_INFO(...) 
	#define HBL2_TRACE(...)
#endif