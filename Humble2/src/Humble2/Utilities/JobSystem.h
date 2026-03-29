#pragma once

// Job system inspired by WickedEngine implementation: https://github.com/turanszkij/WickedEngine/tree/master
// and blog post: https://wickedengine.net/2018/11/simple-job-system-using-standard-c/

#include "Base.h"

#include "Core\Allocators.h"
#include "Allocators\Arena.h"
#include "Collections\Collections.h"

#include "moodycamel/concurrentqueue.h"

#include <mutex>
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

        inline uint32_t GetThreadCount() const { return m_NumThreads; }
        uint32_t GetWorkerIndex();
        Arena* GetWorkerArena();

    private:
        JobSystem() {}

        void InternalInitialize();
        void InternalShutdown();
        void WorkerThreadFunc(uint32_t threadIndex);

        PoolReservation* m_Reservation = nullptr;
        Arena m_JobSystemArena;

        uint32_t m_NumThreads = 0;

        DArray<std::thread> m_Workers = MakeEmptyDArray<std::thread>();
        DArray<Arena*> m_WorkerArenas = MakeEmptyDArray<Arena*>();
        DArray<moodycamel::ConcurrentQueue<std::function<void()>>> m_LocalJobQueues = MakeEmptyDArray<moodycamel::ConcurrentQueue<std::function<void()>>>();

        std::condition_variable m_WakeCondition;
        std::mutex m_WakeMutex;
        std::atomic<bool> m_Shutdown = false;
        std::atomic<uint32_t> m_NextQueue{0};

        static JobSystem* s_Instance;
    };
}