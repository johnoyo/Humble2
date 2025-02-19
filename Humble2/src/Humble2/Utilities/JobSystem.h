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

    struct HBL2_API JobContext
    {
        std::atomic<uint64_t> counter{0};
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
        static bool IsShuttingDown();

        void Execute(JobContext& ctx, const std::function<void()>& job);
        void Dispatch(JobContext& ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job);
        bool Busy(const JobContext& ctx);
        void Wait(const JobContext& ctx);

    private:
        JobSystem() {}

        void InternalInitialize();
        void InternalShutdown();
        void WorkerThreadFunc(uint32_t threadIndex);

        uint32_t m_NumThreads = 0;
        std::vector<std::thread> m_Workers;

        std::vector<ThreadSafeRingBuffer<std::function<void()>, 64>> m_LocalJobQueues;
        ThreadSafeRingBuffer<std::function<void()>, 64> m_GlobalJobQueue;
        std::condition_variable m_WakeCondition;
        std::mutex m_WakeMutex;
        std::atomic<bool> m_Shutdown = false;

        static JobSystem* s_Instance;
    };
}