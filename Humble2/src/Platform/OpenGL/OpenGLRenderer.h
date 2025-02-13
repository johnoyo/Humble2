#pragma once

#include "Base.h"
#include "Core\Context.h"

#include "Scene\Components.h"
#include "Renderer\Renderer.h"
#include "Resources\ResourceManager.h"

#include "OpenGLCommandBuffer.h"
#include "OpenGLDevice.h"
#include "OpenGLResourceManager.h"

#include "Resources\OpenGLBuffer.h"
#include "Resources\OpenGLShader.h"
#include "Resources\OpenGLTexture.h"
#include "Resources\OpenGLFrameBuffer.h"

#include "OpenGLWindow.h"
#include "OpenGLCommon.h"

#include "Utilities\TextureUtilities.h"

namespace HBL2
{
	class OpenGLRenderer final : public Renderer
	{
	public:
		virtual ~OpenGLRenderer() = default;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;
		virtual void Present() override;
		virtual void Clean() override;

		virtual CommandBuffer* BeginCommandRecording(CommandBufferType type, RenderPassStage stage) override;

		virtual void* GetDepthAttachment() override;
		virtual void* GetColorAttachment() override;

		virtual void Draw(Handle<Mesh> mesh) override;
		virtual void DrawIndexed(Handle<Mesh> mesh) override;

		virtual Handle<FrameBuffer> GetMainFrameBuffer() override { return m_MainFrameBuffer; }
		
		virtual Handle<BindGroup> GetGlobalBindings2D() override { return m_GlobalBindings2D; }
		virtual Handle<BindGroup> GetGlobalBindings3D() override { return m_GlobalBindings3D; }
		virtual Handle<BindGroup> GetGlobalPresentBindings() override { return m_GlobalPresentBindings; }

	protected:
		virtual void PreInitialize() override;
		virtual void PostInitialize() override;

	private:
		void CreateBindings();
		void CreateRenderPass();

	private:
		OpenGLResourceManager* m_ResourceManager = nullptr;
		CommandBuffer* m_OpaqueCommandBuffer = nullptr;
		CommandBuffer* m_OpaqueSpriteCommandBuffer = nullptr;
		CommandBuffer* m_PresentCommandBuffer = nullptr;
		CommandBuffer* m_UserInterfaceCommandBuffer = nullptr;

		Handle<BindGroup> m_GlobalBindings2D;
		Handle<BindGroup> m_GlobalBindings3D;
		Handle<BindGroup> m_GlobalPresentBindings;
		Handle<FrameBuffer> m_MainFrameBuffer;
	};
}