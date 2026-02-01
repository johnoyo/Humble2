#pragma once

#include "Resources\ResourceManager.h"

#include "Resources\Pool.h"
#include "Resources\SplitPool.h"
#include "Resources\Types.h"
#include "Resources\TypeDescriptors.h"

#include "Resources\VulkanBuffer.h"
#include "Resources\VulkanShader.h"
#include "Resources\VulkanTexture.h"
#include "Resources\VulkanFrameBuffer.h"
#include "Resources\VulkanBindGroup.h"
#include "Resources\VulkanBindGroupLayout.h"
#include "Resources\VulkanRenderPass.h"
#include "Resources\VulkanRenderPassLayout.h"

namespace HBL2
{
	class VulkanResourceManager final : public ResourceManager
	{
	public:
		virtual ~VulkanResourceManager() = default;

		virtual void Initialize() override;
		virtual void Clean() override;

		// Textures
		virtual Handle<Texture> CreateTexture(const TextureDescriptor&& desc) override;
		virtual void DeleteTexture(Handle<Texture> handle) override;
		virtual void UpdateTexture(Handle<Texture> handle, const Span<const std::byte>& bytes) override;
		virtual void TransitionTextureLayout(CommandBuffer* commandBuffer, Handle<Texture> handle, TextureLayout currentLayout, TextureLayout newLayout, Handle<BindGroup> bindGroupHandle) override;
		virtual glm::vec3 GetTextureDimensions(Handle<Texture> handle) override;
		virtual void* GetTextureData(Handle<Texture> handle) override;
		VulkanTexture* GetTexture(Handle<Texture> handle) const;

		// Buffers
		virtual Handle<Buffer> CreateBuffer(const BufferDescriptor&& desc) override;
		virtual void DeleteBuffer(Handle<Buffer> handle) override;
		virtual void ReAllocateBuffer(Handle<Buffer> handle, uint32_t currentOffset) override;
		virtual void* GetBufferData(Handle<Buffer> handle) override;
		virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) override;
		virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) override;
		virtual void MapBufferData(Handle<Buffer> buffer, intptr_t offset, intptr_t size) override;
		VulkanBuffer GetBuffer(Handle<Buffer> handle) const;
		VulkanBufferHot* GetBufferHot(Handle<Buffer> handle) const;
		VulkanBufferCold* GetBufferCold(Handle<Buffer> handle) const;

		// Framebuffers
		virtual Handle<FrameBuffer> CreateFrameBuffer(const FrameBufferDescriptor&& desc) override;
		virtual void DeleteFrameBuffer(Handle<FrameBuffer> handle) override;
		virtual void ResizeFrameBuffer(Handle<FrameBuffer> handle, uint32_t width, uint32_t height) override;
		VulkanFrameBuffer* GetFrameBuffer(Handle<FrameBuffer> handle) const;

		// Shaders
		virtual Handle<Shader> CreateShader(const ShaderDescriptor&& desc) override;
		virtual void RecompileShader(Handle<Shader> handle, const ShaderDescriptor&& desc) override;
		virtual void DeleteShader(Handle<Shader> handle) override;
		virtual uint64_t GetOrAddShaderVariant(Handle<Shader> handle, const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc) override;
		VulkanShader GetShader(Handle<Shader> handle) const;
		VulkanShaderHot* GetShaderHot(Handle<Shader> handle) const;
		VulkanShaderCold* GetShaderCold(Handle<Shader> handle) const;

		// BindGroups
		virtual Handle<BindGroup> CreateBindGroup(const BindGroupDescriptor&& desc) override;
		virtual void DeleteBindGroup(Handle<BindGroup> handle) override;
		virtual void UpdateBindGroup(Handle<BindGroup> handle);
		virtual uint64_t GetBindGroupHash(Handle<BindGroup> handle) override;
		VulkanBindGroup GetBindGroup(Handle<BindGroup> handle) const;
		VulkanBindGroupHot* GetBindGroupHot(Handle<BindGroup> handle) const;
		VulkanBindGroupCold* GetBindGroupCold(Handle<BindGroup> handle) const;

		// BindGroupsLayouts
		virtual Handle<BindGroupLayout> CreateBindGroupLayout(const BindGroupLayoutDescriptor&& desc) override;
		virtual void DeleteBindGroupLayout(Handle<BindGroupLayout> handle) override;
		VulkanBindGroupLayout* GetBindGroupLayout(Handle<BindGroupLayout> handle) const;

		// RenderPass
		virtual Handle<RenderPass> CreateRenderPass(const RenderPassDescriptor&& desc) override;
		virtual void DeleteRenderPass(Handle<RenderPass> handle) override;
		VulkanRenderPass* GetRenderPass(Handle<RenderPass> handle) const;

		// RenderPassLayouts
		virtual Handle<RenderPassLayout> CreateRenderPassLayout(const RenderPassLayoutDescriptor&& desc) override;
		virtual void DeleteRenderPassLayout(Handle<RenderPassLayout> handle) override;
		VulkanRenderPassLayout* GetRenderPassLayout(Handle<RenderPassLayout> handle) const;

	private:
		Pool<VulkanTexture, Texture> m_TexturePool;
		SplitPool<VulkanBufferHot, VulkanBufferCold, Buffer> m_BufferSplitPool;
		SplitPool<VulkanShaderHot, VulkanShaderCold, Shader> m_ShaderSplitPool;
		Pool<VulkanFrameBuffer, FrameBuffer> m_FrameBufferPool;
		SplitPool<VulkanBindGroupHot, VulkanBindGroupCold, BindGroup> m_BindGroupSplitPool;
		Pool<VulkanBindGroupLayout, BindGroupLayout> m_BindGroupLayoutPool;
		Pool<VulkanRenderPass, RenderPass> m_RenderPassPool;
		Pool<VulkanRenderPassLayout, RenderPassLayout> m_RenderPassLayoutPool;

		friend class VulkanRenderer; // This is required for a hack to create the swapchain images in the VulkanRenderer

	private:
		uint64_t CalculateBindGroupHash(const VulkanBindGroupCold* bindGroupCold);
	};
}
