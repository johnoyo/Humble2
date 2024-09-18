#pragma once
#include <Utilities\Log.h>

namespace HBL2
{
	class Scene;

	enum class SystemType
	{
		Core = 0,
		Runtime,
	};

	class ISystem
	{
	public:
		virtual ~ISystem() = default;

		virtual void OnCreate()				= 0;
		virtual void OnUpdate(float ts)		= 0;
		virtual void OnGuiRender(float ts)	{}
		virtual void OnDestroy()			{}

		void SetContext(Scene* context)
		{
			m_Context = context;
		}

		void SetType(SystemType type)
		{
			m_Type = type;
		}

	protected:
		Scene* m_Context = nullptr;
		SystemType m_Type = SystemType::Core;
	};
}