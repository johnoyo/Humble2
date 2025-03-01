#pragma once

#include "Humble2API.h"

#include "Resources\Handle.h"
#include "Scene\Scene.h"

namespace HBL2
{
	enum class HBL2_API Mode
	{
		None = 0,
		Runtime,
		Editor,
	};

	class HBL2_API Context
	{
	public:
		virtual ~Context() {}

		virtual void OnCreate() {}
		virtual void OnUpdate(float ts) {}
		virtual void OnFixedUpdate() {}
		virtual void OnGuiRender(float ts) {}
		virtual void OnDestroy() {}

		inline static glm::vec2 ViewportSize;
		inline static glm::vec2 ViewportPosition;

		inline static Handle<Scene> ActiveScene;
		inline static Handle<Scene> EmptyScene;
		inline static Handle<Scene> EditorScene;

		inline static Mode Mode = Mode::None;
	};
}