#pragma once

#include "Base.h"

#include <Core\Events.h>

namespace HBL2
{
#ifdef DIST
	#define BEGIN_PROFILE_SYSTEM()
	#define END_PROFILE_SYSTEM(time)
#else
	#define BEGIN_PROFILE_SYSTEM() Timer profileSystem
	#define END_PROFILE_SYSTEM(time) time = profileSystem.ElapsedMillis()
#endif

	class Scene;

	enum class HBL2_API SystemType
	{
		Core = 0,
		Runtime,
		User,
	};

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

		virtual void OnAttach()				{};
		virtual void OnCreate()				= 0;
		virtual void OnUpdate(float ts)		= 0;
		virtual void OnFixedUpdate()        {}
		virtual void OnGuiRender(float ts)	{}
		virtual void OnDestroy()			{}
		virtual void OnDetach()				{}

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

	protected:

		// TODO: Add execution order.

		Scene* m_Context = nullptr;
		SystemType m_Type = SystemType::Core;
		SystemState m_State = SystemState::Idle;
	};
}