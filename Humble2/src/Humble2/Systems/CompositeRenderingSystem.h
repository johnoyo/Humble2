#pragma once

#include "Scene\ISystem.h"
#include "Renderer\Renderer.h"
#include "Resources\ResourceManager.h"

namespace HBL2
{
	class CompositeRenderingSystem final : public ISystem
	{
	public:
		CompositeRenderingSystem() { Name = "CompositeRenderingSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnDestroy() override;

	private:
		Handle<RenderPass> m_RenderPass;
		Handle<FrameBuffer> m_FrameBuffer;
		Handle<Buffer> m_VertexBuffer;
		Handle<Mesh> m_QuadMesh;
		Handle<Material> m_QuadMaterial;
		Handle<BindGroup> m_GlobalPresentBindings;
	};
}