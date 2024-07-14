#pragma once

#include "Base.h"
#include "Core\Context.h"

#include "Scene\Components.h"
#include "Renderer\Rewrite\Renderer.h"
#include "Renderer\Rewrite\ResourceManager.h"
#include "Renderer\Rewrite\OpenGLResourceManager.h"

namespace HBL2
{
	class OpenGLRenderer final : public Renderer
	{
	public:
		virtual ~OpenGLRenderer() = default;

		virtual void Initialize() override;
		virtual void BeginFrame() override;
		virtual void SetPipeline(HBL::Handle<HBL::Material>& material) override;
		virtual void SetBuffers(HBL::Handle<HBL::Mesh>& mesh) override;
		virtual void SetBufferData(HBL::Handle<HBL::Mesh>& mesh) override;
		virtual void SetBindGroups(HBL::Handle<HBL::Material>& material) override;
		virtual void Draw(HBL::Handle<HBL::Mesh>& mesh, HBL::Handle<HBL::Material>& material) override;
		virtual void DrawIndexed(HBL::Handle<HBL::Mesh>& mesh, HBL::Handle<HBL::Material>& material) override;
		virtual void EndFrame() override;
		virtual void Clean() override;

	private:
		HBL::OpenGLResourceManager* m_ResourceManager = nullptr;
	};
}