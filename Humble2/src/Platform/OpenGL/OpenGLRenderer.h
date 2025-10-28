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

		virtual CommandBuffer* BeginCommandRecording(CommandBufferType type) override;

		virtual void* GetDepthAttachment() override;
		virtual void* GetColorAttachment() override;

		virtual void SetViewportAttachment(Handle<Texture> viewportTexture) {}
		virtual void* GetViewportAttachment() override { return nullptr; }

		virtual Handle<FrameBuffer> GetMainFrameBuffer() override { return m_MainFrameBuffer; }
		
		virtual const uint32_t GetFrameIndex() const override { return 0; }

		virtual Handle<BindGroup> GetShadowBindings() override { return m_ShadowBindings; }
		virtual Handle<BindGroup> GetGlobalBindings2D() override { return m_GlobalBindings2D; }
		virtual Handle<BindGroup> GetGlobalBindings3D() override { return m_GlobalBindings3D; }
		virtual Handle<BindGroup> GetGlobalPresentBindings() override { return m_GlobalPresentBindings; }
		virtual Handle<BindGroup> GetDebugBindings() override { return m_DebugBindings; }

	protected:
		virtual void PreInitialize() override;
		virtual void PostInitialize() override;

	private:
		void CreateBindings();
		void CreateRenderPass();
		void Resize(uint32_t width, uint32_t height);

	private:
		OpenGLResourceManager* m_ResourceManager = nullptr;

		CommandBuffer* m_MainCommandBuffer = nullptr;
		CommandBuffer* m_UserInterfaceCommandBuffer = nullptr;

		Handle<BindGroup> m_ShadowBindings;
		Handle<BindGroup> m_GlobalBindings2D;
		Handle<BindGroup> m_GlobalBindings3D;
		Handle<BindGroup> m_GlobalPresentBindings;
		Handle<BindGroup> m_DebugBindings;

		Handle<FrameBuffer> m_MainFrameBuffer;

		bool m_Resize = false;
		glm::uvec2 m_NewSize{};
	};
}