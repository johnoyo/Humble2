#pragma once

#include "Scene\Scene.h"

namespace HBL2
{
	class Context
	{
	public:
		virtual void OnAttach() {}
		virtual void OnCreate() {}
		virtual void OnUpdate(float ts) {}
		virtual void OnGuiRender(float ts) {}
		virtual void OnDetach() {}

		inline static Scene* ActiveScene = nullptr;
		inline static Scene* EmptyScene = nullptr;

	protected:
		std::vector<Scene*> m_Scenes;
	};
}