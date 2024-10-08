#pragma once

#include "Base.h"
#include "Core\Context.h"

#include "Scene\Components.h"
#include "Renderer\Rewrite\Renderer.h"
#include "Resources\ResourceManager.h"

#include "OpenGLResourceManager.h"

#include "OpenGLBuffer.h"
#include "OpenGLShader.h"
#include "OpenGLTexture.h"
#include "OpenGLFrameBuffer.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#include "Platform\OpenGL\Rewrite\OpenGLDebug.h"
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

namespace HBL2
{
	class OpenGLRenderer final : public Renderer
	{
	public:
		virtual ~OpenGLRenderer() = default;

		virtual void Initialize() override;
		virtual void BeginFrame() override;
		virtual void SetPipeline(Handle<Shader> shader) override;
		virtual void SetBuffers(Handle<Mesh> mesh, Handle<Material> material) override;
		virtual void SetBindGroups(Handle<Material> material) override;
		virtual void SetBindGroup(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset, uint32_t size) override;
		virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) override;
		virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) override;
		virtual void WriteBuffer(Handle<Buffer> buffer, intptr_t offset) override;
		virtual void WriteBuffer(Handle<BindGroup> bindGroup, uint32_t bufferIndex) override;
		virtual void WriteBuffer(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset) override;
		virtual void Draw(Handle<Mesh> mesh) override;
		virtual void DrawIndexed(Handle<Mesh> mesh) override;
		virtual CommandBuffer* BeginCommandRecording(CommandBufferType type) override;
		virtual void EndFrame() override;
		virtual void Clean() override;

		virtual void ResizeFrameBuffer(uint32_t width, uint32_t height) override;
		virtual void* GetDepthAttachment() override;
		virtual void* GetColorAttachment() override;

	private:
		OpenGLResourceManager* m_ResourceManager = nullptr;
		CommandBuffer* m_MainCommandBuffer = nullptr;
		CommandBuffer* m_SecondaryCommandBuffer = nullptr;
	};
}