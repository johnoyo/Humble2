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

		inline static Scene* ActiveScene = nullptr;
		inline static Scene* EmptyScene = nullptr;

		inline static Scene* EditorScene = nullptr;

		inline static Mode Mode = Mode::None;
	};
}