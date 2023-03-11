#include "Log.h"

namespace HBL
{
	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	void Log::Initialize()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");

		s_CoreLogger = spdlog::stdout_color_mt("[HUMBLE2]");
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = spdlog::stdout_color_mt("[APP]");
		s_ClientLogger->set_level(spdlog::level::trace);
	}

	std::shared_ptr<spdlog::logger>& Log::GetCoreLogger()
	{
		return s_CoreLogger;
	}

	std::shared_ptr<spdlog::logger>& Log::GetClientLogger()
	{
		return s_ClientLogger;
	}
}