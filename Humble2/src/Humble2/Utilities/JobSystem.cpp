#include "JobSystem.h"

namespace HBL2
{
	void JobSystem::Initialize()
	{
        // Initialize the worker execution state to 0:
        m_FinishedLabel.store(0);

        // Retrieve the number of hardware threads in this system:
        auto numCores = std::thread::hardware_concurrency();

        HBL2_CORE_TRACE("Number of hardware threads in this system: {0}", numCores);

        // Calculate the actual number of worker threads we want:
        m_NumThreads = std::max(1u, numCores);

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

            // *****Here we could do platform specific thread setup...

            worker.detach(); // forget about this thread, let it do it's job in the infinite loop that we created above
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