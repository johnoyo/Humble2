#pragma once

#include "Base.h"
#include "Core\Context.h"

#include "Scene\Components.h"
#include "Renderer\Renderer.h"
#include "Resources\ResourceManager.h"

#include "VulkanResourceManager.h"

namespace HBL2
{
	class VulkanRenderer final : public Renderer
	{
	public:
		virtual ~VulkanRenderer() = default;

		virtual void Initialize() override;
		virtual void BeginFrame() override;
		virtual void EndFrame() override;
		virtual void Clean() override;

		virtual void SetPipeline(Handle<Shader> shader) override {}
		virtual void SetBuffers(Handle<Mesh> mesh, Handle<Material> material) override {}
		virtual void SetBindGroups(Handle<Material> material) override {}
		virtual void SetBindGroup(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset, uint32_t size) override {}
		virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) override {}
		virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) override {}
		virtual void WriteBuffer(Handle<Buffer> buffer, intptr_t offset) override {}
		virtual void WriteBuffer(Handle<BindGroup> bindGroup, uint32_t bufferIndex) override {}
		virtual void WriteBuffer(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset) override {}
		virtual void Draw(Handle<Mesh> mesh) override {}
		virtual void DrawIndexed(Handle<Mesh> mesh) override {}
		virtual CommandBuffer* BeginCommandRecording(CommandBufferType type) override { return nullptr; }

		virtual void ResizeFrameBuffer(uint32_t width, uint32_t height) override {}
		virtual void* GetDepthAttachment() override { return nullptr; }
		virtual void* GetColorAttachment() override { return nullptr; }
	};
}