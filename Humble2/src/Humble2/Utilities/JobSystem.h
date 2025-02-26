#pragma once

// Job system inspired by WickedEngine implementation: https://github.com/turanszkij/WickedEngine/tree/master
// and blog post: https://wickedengine.net/2018/11/simple-job-system-using-standard-c/

#include "Base.h"

#include <deque>
#include <mutex>
#include <atomic>
#include <thread>
#include <algorithm>
#include <functional>
#include <condition_variable>

namespace HBL2
{
    template <typename T>
    class ThreadSafeDeque
    {
    public:
        ThreadSafeDeque() {}
        ThreadSafeDeque(const ThreadSafeDeque<T>& other) : m_Data(other.m_Data) {}

        inline void PushBack(const T& item)
        {
            std::scoped_lock lock(m_Lock);
            m_Data.push_back(item);
        }

        inline bool PopFront(T& item)
        {
            std::scoped_lock lock(m_Lock);

            if (m_Data.empty())
            {
                return false;
            }

            item = std::move(m_Data.front());
            m_Data.pop_front();
            return true;
        }

        inline void Reset()
        {
            std::scoped_lock lock(m_Lock);
            m_Data.clear();
        }

    private:
        std::deque<T> m_Data;
        std::mutex m_Lock;
    };

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

        std::vector<ThreadSafeDeque<std::function<void()>>> m_LocalJobQueues;
        std::condition_variable m_WakeCondition;
        std::mutex m_WakeMutex;
        std::atomic<bool> m_Shutdown = false;
        std::atomic<uint32_t> m_NextQueue{0};

        static JobSystem* s_Instance;
    };
}