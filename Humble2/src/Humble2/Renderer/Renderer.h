#pragma once

#include "Resources\Types.h"
#include "Resources\Handle.h"
#include "CommandBuffer.h"
#include "UniformRingBuffer.h"

namespace HBL2
{
	enum class GraphicsAPI
	{
		NONE,
		OPENGL,
		VULKAN,
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

		virtual void SetPipeline(Handle<Shader> shader) = 0;
		virtual void SetBuffers(Handle<Mesh> mesh, Handle<Material> material) = 0;
		virtual void SetBindGroups(Handle<Material> material) = 0;
		virtual void SetBindGroup(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset, uint32_t size) = 0;
		virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) = 0;
		virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) = 0;
		virtual void WriteBuffer(Handle<Buffer> buffer, intptr_t offset) = 0;
		virtual void WriteBuffer(Handle<BindGroup> bindGroup, uint32_t bufferIndex) = 0;
		virtual void WriteBuffer(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset) = 0;

		virtual void Draw(Handle<Mesh> mesh) = 0;
		virtual void DrawIndexed(Handle<Mesh> mesh) = 0;

		////
#ifdef false

		virtual void SetIndexBuffer(Handle<Buffer> mesh) = 0;
		virtual void SetVertexBuffer(Handle<Shader> shader, Handle<Buffer> mesh) = 0;
		virtual void SetShader(Handle<Shader> shader) = 0;
		virtual void SetBindGroup(Handle<BindGroup> bindGroup) = 0;

		virtual void WriteBuffer(Handle<Buffer> buffer, void* newData, intptr_t offset) = 0;
		virtual void WriteTexture(Handle<Texture> texture) = 0;

		virtual void CopyBufferToBuffer(Handle<Buffer> srcBuffer, Handle<Buffer> dstBuffer, intptr_t offset, intptr_t size) = 0;
		virtual void CopyBufferToTexture(Handle<Buffer> srcBuffer, Handle<Texture> dstTxture) = 0;
		virtual void CopyTextureToBuffer(Handle<Texture> srcTxture, Handle<Buffer> dstBuffer) = 0;

		virtual void EndCommandRecording() = 0;
		virtual void SubmitCommandBuffer() = 0;

		virtual void BeginRenderPass(Handle<RenderPass> renderPass) = 0;
		virtual void EndRenderPass(Handle<RenderPass> renderPass) = 0;
#endif
		////

		virtual CommandBuffer* BeginCommandRecording(CommandBufferType type) = 0;

		virtual void* GetDepthAttachment() = 0;
		virtual void* GetColorAttachment() = 0;

		GraphicsAPI GetAPI() const { return m_GraphicsAPI; }

		UniformRingBuffer* TempUniformRingBuffer;

	protected:
		GraphicsAPI m_GraphicsAPI;
	};
}