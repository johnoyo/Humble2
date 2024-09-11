#pragma once
#include <Utilities\Log.h>

namespace HBL2
{
	class Scene;

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

	protected:
		Scene* m_Context = nullptr;
	};
}