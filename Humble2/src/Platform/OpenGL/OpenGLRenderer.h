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

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#include "Platform\OpenGL\OpenGLDebug.h"
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
		virtual void EndFrame() override;
		virtual void Present() override;
		virtual void Clean() override;

		virtual CommandBuffer* BeginCommandRecording(CommandBufferType type) override;

		virtual void* GetDepthAttachment() override;
		virtual void* GetColorAttachment() override;

		virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) override;
		virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) override;

		virtual void Draw(Handle<Mesh> mesh) override;
		virtual void DrawIndexed(Handle<Mesh> mesh) override;

	private:
		OpenGLResourceManager* m_ResourceManager = nullptr;
		CommandBuffer* m_MainCommandBuffer = nullptr;
		CommandBuffer* m_OffscreenCommandBuffer = nullptr;
		CommandBuffer* m_UserInterfaceCommandBuffer = nullptr;
	};
}