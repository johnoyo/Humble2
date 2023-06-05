#pragma once

namespace HBL2
{
	class Scene;

	class ISystem
	{
	public:
		virtual void OnCreate()				= 0;
		virtual void OnUpdate(float ts)		= 0;
		virtual void OnGuiRender(float ts)	{}
		virtual void OnDestroy()			{}

	protected:
		Scene* m_Context = nullptr;
	};
}