#pragma once

#include "Humble2API.h"

// Ignore all warnings raised inside those headers
#pragma warning(push, 0)
#include <spdlog\spdlog.h>
#include <spdlog\fmt\ostr.h>
#include <spdlog\sinks\stdout_color_sinks.h>
#pragma warning(pop)

#include <memory>

namespace HBL2
{
	class HBL2_API Log
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

#ifdef DEBUG
	#define HBL2_CORE_FATAL(...) ::HBL2::Log::GetCoreLogger()->critical(__VA_ARGS__)
	#define HBL2_CORE_ERROR(...) ::HBL2::Log::GetCoreLogger()->error(__VA_ARGS__)
	#define HBL2_CORE_WARN(...)  ::HBL2::Log::GetCoreLogger()->warn(__VA_ARGS__)
	#define HBL2_CORE_INFO(...)  ::HBL2::Log::GetCoreLogger()->info(__VA_ARGS__)
	#define HBL2_CORE_TRACE(...) ::HBL2::Log::GetCoreLogger()->trace(__VA_ARGS__)

	#define HBL2_FATAL(...) ::HBL2::Log::GetClientLogger()->critical(__VA_ARGS__)
	#define HBL2_ERROR(...) ::HBL2::Log::GetClientLogger()->error(__VA_ARGS__)
	#define HBL2_WARN(...)  ::HBL2::Log::GetClientLogger()->warn(__VA_ARGS__)
	#define HBL2_INFO(...)  ::HBL2::Log::GetClientLogger()->info(__VA_ARGS__)
	#define HBL2_TRACE(...) ::HBL2::Log::GetClientLogger()->trace(__VA_ARGS__)
#elif RELEASE
	#define HBL2_CORE_FATAL(...) ::HBL2::Log::GetCoreLogger()->critical(__VA_ARGS__)
	#define HBL2_CORE_ERROR(...) ::HBL2::Log::GetCoreLogger()->error(__VA_ARGS__)
	#define HBL2_CORE_WARN(...)  ::HBL2::Log::GetCoreLogger()->warn(__VA_ARGS__)
	#define HBL2_CORE_INFO(...)  ::HBL2::Log::GetCoreLogger()->info(__VA_ARGS__)
	#define HBL2_CORE_TRACE(...) ::HBL2::Log::GetCoreLogger()->trace(__VA_ARGS__)

	#define HBL2_FATAL(...) ::HBL2::Log::GetClientLogger()->critical(__VA_ARGS__)
	#define HBL2_ERROR(...) ::HBL2::Log::GetClientLogger()->error(__VA_ARGS__)
	#define HBL2_WARN(...)  ::HBL2::Log::GetClientLogger()->warn(__VA_ARGS__)
	#define HBL2_INFO(...)  ::HBL2::Log::GetClientLogger()->info(__VA_ARGS__)
	#define HBL2_TRACE(...) ::HBL2::Log::GetClientLogger()->trace(__VA_ARGS__)
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