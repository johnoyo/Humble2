#pragma once

#include "../Base.h"
#include "ThreadSafeRingBuffer.h"

#include <atomic>    
#include <thread>    
#include <algorithm> 
#include <functional>
#include <condition_variable>

namespace HBL
{
	class JobSystem
	{
	public:
		JobSystem(const JobSystem&) = delete;

		static JobSystem& Get()
		{
			static JobSystem instance;
			return instance;
		}

		void Initialize();
		void Execute(const std::function<void()>& job);
		bool Busy();
		void Wait();

	private:
		JobSystem() {}

		uint32_t m_NumThreads = 0;
		ThreadSafeRingBuffer<std::function<void()>, 256> m_JobPool;
		std::condition_variable m_WakeCondition;
		std::mutex m_WakeMutex;
		uint64_t m_CurrentLabel = 0;
		std::atomic<uint64_t> m_FinishedLabel;
	};
}