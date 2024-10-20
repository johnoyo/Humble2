#pragma once

#include "Scene\Scene.h"

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

		virtual void OnCreate() {}
		virtual void OnUpdate(float ts) {}
		virtual void OnGuiRender(float ts) {}
		virtual void OnDestroy() {}

		inline static Handle<Scene> ActiveScene;
		inline static Handle<Scene> EmptyScene;
		inline static Handle<Scene> EditorScene;

		inline static Mode Mode = Mode::None;
	};
}