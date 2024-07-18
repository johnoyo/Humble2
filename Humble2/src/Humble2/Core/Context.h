#pragma once

#include "Scene\Scene.h"
#include "Project\Project.h"

namespace HBL2
{
	enum class Mode
	{
		None = 0,
		Runtime,
		Editor,
	};

	class Context
	{
	public:
		virtual ~Context() {}

		virtual void OnAttach() {}
		virtual void OnCreate() {}
		virtual void OnUpdate(float ts) {}
		virtual void OnGuiRender(float ts) {}
		virtual void OnDetach() {}

		inline static Scene* ActiveScene = nullptr;
		inline static Scene* EmptyScene = nullptr;

		inline static Scene* Core = nullptr;

		inline static Mode Mode = Mode::None;

		void AddScene(Scene* scene) { m_Scenes.push_back(scene); }

	protected:
		std::vector<Scene*> m_Scenes;
	};
}