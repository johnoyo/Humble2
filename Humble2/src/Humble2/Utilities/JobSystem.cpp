#include "JobSystem.h"

namespace HBL2
{
    JobSystem* JobSystem::s_Instance = nullptr;

	void JobSystem::Initialize()
	{
        HBL2_CORE_ASSERT(s_Instance == nullptr, "JobSystem::s_Instance is not null! JobSystem::Initialize has been called twice.");
        s_Instance = new JobSystem;

        Get().InternalInitialize();
	}

    void JobSystem::Shutdown()
    {
        HBL2_CORE_ASSERT(s_Instance != nullptr, "JobSystem::s_Instance is null!");

        Get().InternalShutdown();

        delete s_Instance;
        s_Instance = nullptr;
    }

    bool JobSystem::IsShuttingDown()
    {
        return Get().m_Shutdown.load() == true;
    }

    void JobSystem::InternalInitialize()
    {
        m_NumThreads = std::max(1u, std::thread::hardware_concurrency() - 1);

        m_LocalJobQueues.resize(m_NumThreads);
        m_Workers.reserve(m_NumThreads);

        for (uint32_t threadID = 0; threadID < m_NumThreads; ++threadID)
        {
            std::thread& worker = m_Workers.emplace_back([this, threadID] { WorkerThreadFunc(threadID); });

            auto handle = worker.native_handle();

            // Put threads on increasing cores starting from 2nd.
            int core = threadID + 1;
#ifdef _WIN32
            // Put each thread on to dedicated core.
            DWORD_PTR affinityMask = 1ull << core;
            DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
            assert(affinity_result > 0);

            // Set priority to normal.
            BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_NORMAL);
            assert(priority_result != 0);

            // Give a name to each thread for easier debugging.
            std::wstring wthreadname = L"HBL2::Job_" + std::to_wstring(threadID);
            HRESULT hr = SetThreadDescription(handle, wthreadname.c_str());
            assert(SUCCEEDED(hr));
#endif
        }
    }

    void JobSystem::InternalShutdown()
    {
        if (JobSystem::IsShuttingDown())
        {
            return;
        }

        m_Shutdown.store(true);
        m_WakeCondition.notify_all();

        for (std::thread& worker : m_Workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }

    void JobSystem::Execute(JobContext& ctx, const std::function<void()>& job)
    {
        ctx.counter.fetch_add(1, std::memory_order_relaxed);

        auto wrappedJob = [job, &ctx]()
        {
            job();
            ctx.counter.fetch_sub(1, std::memory_order_release);
        };

        uint32_t threadID = m_NextQueue.fetch_add(1, std::memory_order_relaxed) % m_NumThreads;
        m_LocalJobQueues[threadID].PushBack(wrappedJob);

        m_WakeCondition.notify_one();
    }

    void JobSystem::Dispatch(JobContext& ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job)
    {
        if (jobCount == 0 || groupSize == 0)
        {
            return;
        }

        uint32_t groupCount = (jobCount + groupSize - 1) / groupSize;
        ctx.counter.fetch_add(groupCount, std::memory_order_acquire);

        for (uint32_t i = 0; i < groupCount; ++i)
        {
            auto task = [=, &ctx]()
            {
                uint32_t start = i * groupSize;
                uint32_t end = std::min(start + groupSize, jobCount);
                for (uint32_t j = start; j < end; ++j)
                {
                    job(JobDispatchArgs{ j, i });
                }

                ctx.counter.fetch_sub(1, std::memory_order_acq_rel);
            };

            uint32_t threadID = i % m_NumThreads;
            m_LocalJobQueues[threadID].PushBack(task);
        }

        // Wake up all worker threads to start processing the work.
        m_WakeCondition.notify_all();
    }

    bool JobSystem::Busy(const JobContext& ctx)
    {
        return ctx.counter.load(std::memory_order_acquire) > 0;
    }

    void JobSystem::Wait(const JobContext& ctx)
    {
        if (Busy(ctx))
        {
            // Wake up all worker threads to ensure work is being processed.
            m_WakeCondition.notify_all();

            std::function<void()> job;

            // Try to pick up jobs from other threads while waiting.
            for (uint32_t i = 0; i < m_NumThreads; ++i)
            {
                while (m_LocalJobQueues[i].PopFront(job))
                {
                    job();
                }
            }

            // This thread did not find any available jobs to pick up.
            while (Busy(ctx))
            {
                // If we are here, then there are still remaining jobs that this thread couldn't pick up.
                //	In this case those jobs are not standing by on a queue but currently executing
                //	on other threads, so they cannot be picked up by this thread.
                //	Allow to swap out this thread by OS to not spin endlessly for nothing
                std::this_thread::yield();
            }
        }
    }

    void JobSystem::WorkerThreadFunc(uint32_t threadIndex)
    {
        while (!m_Shutdown.load())
        {
            std::function<void()> job;
            bool jobFound = false;

            // 1️. First, try to pop from the local queue
            if (m_LocalJobQueues[threadIndex].PopFront(job))
            {
                jobFound = true;
            }
            else
            {
                // 2️. If no job found in local queue, steal from another thread
                for (uint32_t i = 0; i < m_NumThreads; ++i)
                {
                    uint32_t victimThread = (threadIndex + i + 1) % m_NumThreads;
                    if (m_LocalJobQueues[victimThread].PopFront(job))
                    {
                        jobFound = true;
                        break;
                    }
                }
            }

            if (jobFound)
            {
                job();
            }
            else
            {
                std::unique_lock<std::mutex> lock(m_WakeMutex);
                m_WakeCondition.wait(lock);
            }
        }
    }
}