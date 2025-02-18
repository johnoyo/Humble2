#pragma once

#include "Base.h"
#include "ThreadSafeRingBuffer.h"

#include <atomic>    
#include <thread>    
#include <algorithm> 
#include <functional>
#include <condition_variable>

namespace HBL2
{
	struct HBL2_API JobDispatchArgs
	{
		uint32_t jobIndex;
		uint32_t groupIndex;
	};

	class HBL2_API JobSystem
	{
	public:
		JobSystem(const JobSystem&) = delete;

		static JobSystem& Get()
		{
			HBL2_CORE_ASSERT(s_Instance != nullptr, "JobSystem::s_Instance is null! Call JobSystem::Initialize before use.");
			return *s_Instance;
		}

		static void Initialize();
		static void Shutdown();

		void Execute(const std::function<void()>& job);
		void Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job);
		bool Busy();
		void Wait();

	private:
		JobSystem() {}

		void InitializeInternal();

		uint32_t m_NumThreads = 0;
		ThreadSafeRingBuffer<std::function<void()>, 256> m_JobPool;
		std::condition_variable m_WakeCondition;
		std::mutex m_WakeMutex;
		uint64_t m_CurrentLabel = 0;
		std::atomic<uint64_t> m_FinishedLabel;

		inline static JobSystem* s_Instance = nullptr;
	};
}