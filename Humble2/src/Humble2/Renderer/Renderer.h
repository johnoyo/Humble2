#pragma once

#include "Resources\Types.h"
#include "Resources\Handle.h"
#include "CommandBuffer.h"
#include "UniformRingBuffer.h"

namespace HBL2
{
	struct CameraData
	{
		glm::mat4 ViewProjection;
	};

	struct LightData
	{
		glm::vec4 ViewPosition;
		glm::vec4 LightPositions[16];
		glm::vec4 LightColors[16];
		glm::vec4 LightIntensities[16];
		float LightCount;
		float _padding[3];
	};

	enum class GraphicsAPI
	{
		NONE,
		OPENGL,
		VULKAN,
		WEBGPU,
	};

	class Renderer
	{
	public:
		static inline Renderer* Instance;

		virtual ~Renderer() = default;

		virtual void Initialize() = 0;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void Present() = 0;
		virtual void Clean() = 0;

		virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) = 0;
		virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) = 0;

		virtual void Draw(Handle<Mesh> mesh) = 0;
		virtual void DrawIndexed(Handle<Mesh> mesh) = 0;

		virtual CommandBuffer* BeginCommandRecording(CommandBufferType type) = 0;

		virtual void* GetDepthAttachment() = 0;
		virtual void* GetColorAttachment() = 0;

		virtual Handle<RenderPass> GetMainRenderPass() = 0;
		virtual Handle<FrameBuffer> GetMainFrameBuffer() = 0;
		virtual Handle<BindGroup> GetGlobalBindings2D() = 0;
		virtual Handle<BindGroup> GetGlobalBindings3D() = 0;

		Handle<BindGroupLayout> GetGlobalBindingsLayout2D() { return m_GlobalBindingsLayout2D; }
		Handle<BindGroupLayout> GetGlobalBindingsLayout3D() { return m_GlobalBindingsLayout3D; }

		GraphicsAPI GetAPI() const { return m_GraphicsAPI; }
		DrawList& GetDrawList2D() { return m_DrawList2D; }
		DrawList& GetDrawList3D() { return m_DrawList3D; }

		UniformRingBuffer* TempUniformRingBuffer;

	protected:
		GraphicsAPI m_GraphicsAPI;
		DrawList m_DrawList2D;
		DrawList m_DrawList3D;
		Handle<BindGroupLayout> m_GlobalBindingsLayout2D;
		Handle<BindGroupLayout> m_GlobalBindingsLayout3D;
	};
}