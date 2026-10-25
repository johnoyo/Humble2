#pragma once

#include "Humble2.h"
#include "EditorComponents.h"

namespace HBL2
{
	namespace Editor
	{
		class SceneViewRenderingSystem final : public ISystem
		{
		public:
			virtual void OnCreate() override;
			virtual void OnUpdate(float ts) override;
			virtual void OnRender(CommandBuffer* commandBuffer, const RendererFrameData* renderData) override;
			virtual void OnDestroy() override;

		private:
			ResourceManager* m_ResourceManager = nullptr;
			Scene* m_RuntimeScene = nullptr;
			SceneRenderer* m_SceneRenderer = nullptr;
			UUID m_RendererResizeEventHandle = 0;
		};
	}
}