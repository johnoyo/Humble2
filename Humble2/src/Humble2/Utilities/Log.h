#pragma once

#include "spdlog\spdlog.h"
#include "spdlog\sinks\stdout_color_sinks.h"

#include <memory>

namespace HBL
{
	class Log 
	{
	public:
		static void Initialize();
		static std::shared_ptr<spdlog::logger>& GetCoreLogger();
		static std::shared_ptr<spdlog::logger>& GetClientLogger();
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

#ifndef DEBUG
	#define HBL_CORE_FATAL(...) ::HBL::Log::GetCoreLogger()->critical(__VA_ARGS__)
	#define HBL_CORE_ERROR(...) ::HBL::Log::GetCoreLogger()->error(__VA_ARGS__)
	#define HBL_CORE_WARN(...)  ::HBL::Log::GetCoreLogger()->warn(__VA_ARGS__)
	#define HBL_CORE_INFO(...)  ::HBL::Log::GetCoreLogger()->info(__VA_ARGS__)
	#define HBL_CORE_TRACE(...) ::HBL::Log::GetCoreLogger()->trace(__VA_ARGS__)

	#define HBL_FATAL(...) ::HBL::Log::GetClientLogger()->critical(__VA_ARGS__)
	#define HBL_ERROR(...) ::HBL::Log::GetClientLogger()->error(__VA_ARGS__)
	#define HBL_WARN(...)  ::HBL::Log::GetClientLogger()->warn(__VA_ARGS__)
	#define HBL_INFO(...)  ::HBL::Log::GetClientLogger()->info(__VA_ARGS__)
	#define HBL_TRACE(...) ::HBL::Log::GetClientLogger()->trace(__VA_ARGS__)
#else
	#define HBL_CORE_FATAL(...)
	#define HBL_CORE_ERROR(...)
	#define HBL_CORE_WARN(...) 
	#define HBL_CORE_INFO(...) 
	#define HBL_CORE_TRACE(...)

	#define HBL_FATAL(...)
	#define HBL_ERROR(...)
	#define HBL_WARN(...) 
	#define HBL_INFO(...) 
	#define HBL_TRACE(...)
#endif