#include "Log.h"

#include "Base.h"

namespace HBL2
{
	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> Log::s_ConsoleSink;
	std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> Log::s_FileSink;

	void Log::Initialize()
	{
		// Async thread pool (do this ONCE in the app lifetime)
		spdlog::init_thread_pool(8192, 1);

		// Create sinks
		s_ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		s_ConsoleSink->set_pattern("%^[%T] %n: %v%$");

		s_FileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("Console.log", 5_MB, 3);
		s_FileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");

		// Default: console on, file on (adjust to taste)
		s_ConsoleSink->set_level(spdlog::level::trace);
		s_FileSink->set_level(spdlog::level::trace);

		// Each logger has BOTH sinks
		spdlog::sinks_init_list sinks{ s_ConsoleSink, s_FileSink };

		s_CoreLogger = std::make_shared<spdlog::async_logger>("HUMBLE2", sinks, spdlog::thread_pool(), spdlog::async_overflow_policy::overrun_oldest);

		s_ClientLogger = std::make_shared<spdlog::async_logger>("APP", sinks, spdlog::thread_pool(), spdlog::async_overflow_policy::overrun_oldest);

		s_CoreLogger->set_level(spdlog::level::trace);
		s_ClientLogger->set_level(spdlog::level::trace);

		// Flush policy (common)
		s_CoreLogger->flush_on(spdlog::level::err);
		s_ClientLogger->flush_on(spdlog::level::err);

		spdlog::register_logger(s_CoreLogger);
		spdlog::register_logger(s_ClientLogger);
	}

	void Log::Shutdown()
	{
		if (s_CoreLogger)
		{
			s_CoreLogger->flush();
		}

		if (s_ClientLogger)
		{
			s_ClientLogger->flush();
		}

		spdlog::shutdown();

		s_CoreLogger.reset();
		s_ClientLogger.reset();
	}

	void Log::SetOutputs(BitFlags<LogContexts> outputs)
	{
		const bool consoleOn = outputs.IsSet(LogContexts::TERMINAL);
		const bool fileOn = outputs.IsSet(LogContexts::FILE);

		if (s_ConsoleSink)
		{
			s_ConsoleSink->set_level(consoleOn ? spdlog::level::trace : spdlog::level::off);
		}

		if (s_FileSink)
		{
			s_FileSink->set_level(fileOn ? spdlog::level::trace : spdlog::level::off);
		}
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