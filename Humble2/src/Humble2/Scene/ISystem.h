#pragma once

#include "Base.h"

#include "Scene.h"
#include "IJob.h"

#include "Core\Events.h"

#include <vector>
#include <bitset>
#include <functional>
#include <algorithm>
#include <typeindex>
#include <iostream>

namespace HBL2
{
#ifdef DIST
	#define BEGIN_PROFILE_SYSTEM()
	#define END_PROFILE_SYSTEM(time)
#else
	#define BEGIN_PROFILE_SYSTEM() Timer profileSystem
	#define END_PROFILE_SYSTEM(time) time = profileSystem.ElapsedMillis()
#endif

	enum class HBL2_API SystemState
	{
		Play = 0,
		Pause,
		Idle,
	};

	class HBL2_API ISystem
	{
	public:
		virtual ~ISystem() = default;

		virtual void OnAttach()				 {};
		virtual void OnCreate()				 = 0;
		virtual void OnUpdate(float ts)		 = 0;
		virtual void OnFixedUpdate()         {}
		virtual void OnGuiRender(float ts)	 {}
		virtual void OnGizmoRender(float ts) {}
		virtual void OnDestroy()			 {}
		virtual void OnDetach()				 {}

		void SetContext(Scene* context)
		{
			m_Context = context;
		}

		const Scene* GetContext() const
		{
			return m_Context;
		}

		void SetType(SystemType type)
		{
			m_Type = type;
		}

		const SystemType GetType() const
		{
			return m_Type;
		}

		void SetState(SystemState newState)
		{
			SystemState previousState = m_State;
			m_State = newState;
			EventDispatcher::Get().Post(SystemStateChangeEvent(this, previousState, newState));
		}

		const SystemState GetState() const
		{
			return m_State;
		}

		std::string Name = "UnnamedSystem";
		float RunningTime = 0.f;
		int32_t ExecutionOrder = 1000;

	protected:
		template<typename T>
		void AppendJob(Scene* ctx)
		{
			IJob* job = new T;
			job->Resolve();
			job->SetContext(ctx);

			m_Jobs.push_back(job);
		}

		void ScheduleJobs()
		{
			std::vector<std::vector<JobEntry>> batches;

			ComponentMaskType batchReadMask;
			ComponentMaskType batchWriteMask;

			batches.emplace_back();
			std::vector<JobEntry>* currentBatch = &batches.back();

			for (IJob* job : m_Jobs)
			{
				const auto& jobCtx = job->GetEntry();

				// Check conflicts with current batch.
				bool conflict =
					(jobCtx.WriteMask & batchWriteMask).any() ||  // WW conflict
					(jobCtx.WriteMask & batchReadMask).any() ||   // WR conflict
					(jobCtx.ReadMask & batchWriteMask).any();     // RW conflict

				if (conflict)
				{
					// Start new batch
					batches.emplace_back();
					batchReadMask.reset();
					batchWriteMask.reset();
					currentBatch = &batches.back();
				}

				// Add system to current batch.
				currentBatch->push_back(jobCtx);

				// Update read and write mask for current batch.
				batchReadMask |= jobCtx.ReadMask;
				batchWriteMask |= jobCtx.WriteMask;
			}

			// Execute batches
			for (const auto& batch : batches)
			{
				if (batch.size() == 1)
				{
					batch[0].Job->Execute();
				}
				else
				{
					JobContext ctx;
					for (const auto& e : batch)
					{
						IJob* job = e.Job;
						JobSystem::Get().Execute(ctx, [job]() { job->Execute(); });
					}
					JobSystem::Get().Wait(ctx);
				}
			}

			m_Jobs.clear();
		}

	protected:
		Scene* m_Context = nullptr;
		SystemType m_Type = SystemType::Core;
		SystemState m_State = SystemState::Idle;
		std::vector<IJob*> m_Jobs;
	};	
}