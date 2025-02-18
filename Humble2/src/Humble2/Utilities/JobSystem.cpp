#include "JobSystem.h"

namespace HBL2
{
	void JobSystem::Initialize()
	{
        HBL2_CORE_ASSERT(s_Instance == nullptr, "JobSystem::s_Instance is not null! JobSystem::Initialize has been called twice.");
        s_Instance = new JobSystem;

        Get().InitializeInternal();
	}

    void JobSystem::Shutdown()
    {
        HBL2_CORE_ASSERT(s_Instance != nullptr, "JobSystem::s_Instance is null!");

        delete s_Instance;
        s_Instance = nullptr;
    }

    void JobSystem::InitializeInternal()
    {
        // Initialize the worker execution state to 0:
        m_FinishedLabel.store(0);

        // Retrieve the number of hardware threads in this system:
        auto numCores = std::thread::hardware_concurrency();

        HBL2_CORE_TRACE("Number of hardware threads in this system: {0}", numCores);

        // Calculate the actual number of worker threads we want:
        m_NumThreads = std::max(1u, numCores - 1);

        // Create all our worker threads while immediately starting them:
        for (uint32_t threadID = 0; threadID < m_NumThreads; ++threadID)
        {
            std::thread worker([this]
            {
                std::function<void()> job;

                // This is the infinite loop that a worker thread will do 
                while (true)
                {
                    if (m_JobPool.pop_front(job)) // try to grab a job from the jobPool queue
                    {
                        // It found a job, execute it:
                        job(); // execute job
                        m_FinishedLabel.fetch_add(1); // update worker label state
                    }
                    else
                    {
                        // no job, put thread to sleep
                        std::unique_lock<std::mutex> lock(m_WakeMutex);
                        m_WakeCondition.wait(lock);
                    }
                }
            });

            auto handle = worker.native_handle();

            // Put threads on increasing cores starting from 2nd.
            int core = threadID + 1;

#ifdef _WIN32
            // Put each thread on to dedicated core:
            DWORD_PTR affinityMask = 1ull << core;
            DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
            assert(affinity_result > 0);

            BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_NORMAL);
            assert(priority_result != 0);

            std::wstring wthreadname = L"HBL2::Job_" + std::to_wstring(threadID);
            HRESULT hr = SetThreadDescription(handle, wthreadname.c_str());
            assert(SUCCEEDED(hr));
#endif
            worker.detach();
        }
    }

	void JobSystem::Execute(const std::function<void()>& job)
	{
        // The main thread label state is updated:
        m_CurrentLabel += 1;

        // Try to push a new job until it is pushed successfully:
        while (!m_JobPool.push_back(job)) 
        { 
            m_WakeCondition.notify_one();
            std::this_thread::yield();
        }

        m_WakeCondition.notify_one();
	}

    void JobSystem::Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job)
    {
        if (jobCount == 0 || groupSize == 0)
        {
            return;
        }

        // Calculate the amount of job groups to dispatch (overestimate, or "ceil"):
        const uint32_t groupCount = (jobCount + groupSize - 1) / groupSize;

        // The main thread label state is updated:
        m_CurrentLabel += groupCount;

        for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
        {
            // For each group, generate one real job:
            const auto& jobGroup = [jobCount, groupSize, job, groupIndex]()
            {
                // Calculate the current group's offset into the jobs:
                const uint32_t groupJobOffset = groupIndex * groupSize;
                const uint32_t groupJobEnd = std::min(groupJobOffset + groupSize, jobCount);

                JobDispatchArgs args;
                args.groupIndex = groupIndex;

                // Inside the group, loop through all job indices and execute job for each index:
                for (uint32_t i = groupJobOffset; i < groupJobEnd; ++i)
                {
                    args.jobIndex = i;
                    job(args);
                }
            };

            // Try to push a new job until it is pushed successfully:
            while (!m_JobPool.push_back(jobGroup))
            {
                m_WakeCondition.notify_one();
                std::this_thread::yield();
            }

            m_WakeCondition.notify_one(); // wake one thread
        }
    }

	bool JobSystem::Busy()
	{
		return m_FinishedLabel.load() < m_CurrentLabel;
	}

	void JobSystem::Wait()
	{
        while (Busy())
        { 
            m_WakeCondition.notify_one();
            std::this_thread::yield();
        }
	}
}