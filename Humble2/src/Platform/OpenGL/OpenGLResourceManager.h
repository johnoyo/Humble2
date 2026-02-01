#pragma once

#include "Resources\ResourceManager.h"

#include "Resources\Pool.h"
#include "Resources\Types.h"
#include "Resources\TypeDescriptors.h"

#include "Resources\OpenGLBuffer.h"
#include "Resources\OpenGLShader.h"
#include "Resources\OpenGLTexture.h"
#include "Resources\OpenGLFrameBuffer.h"
#include "Resources\OpenGLBindGroup.h"
#include "Resources\OpenGLBindGroupLayout.h"
#include "Resources\OpenGLRenderPass.h"
#include "Resources\OpenGLRenderPassLayout.h"

namespace HBL2
{
	class OpenGLResourceManager final : public ResourceManager
	{
	public:
		virtual ~OpenGLResourceManager() = default;

		virtual void Initialize() override;
		virtual void Clean() override;

		// Textures
		virtual Handle<Texture> CreateTexture(const TextureDescriptor&& desc) override;
		virtual void DeleteTexture(Handle<Texture> handle) override;
		virtual void UpdateTexture(Handle<Texture> handle, const Span<const std::byte>& bytes) override;
		virtual void TransitionTextureLayout(CommandBuffer* commandBuffer, Handle<Texture> handle, TextureLayout currentLayout, TextureLayout newLayout, Handle<BindGroup> bindGroupHandle) override;
		virtual glm::vec3 GetTextureDimensions(Handle<Texture> handle) override;
		virtual void* GetTextureData(Handle<Texture> handle) override;
		OpenGLTexture* GetTexture(Handle<Texture> handle) const;

		// Buffers
		virtual Handle<Buffer> CreateBuffer(const BufferDescriptor&& desc) override;
		virtual void DeleteBuffer(Handle<Buffer> handle) override;
		virtual void ReAllocateBuffer(Handle<Buffer> handle, uint32_t currentOffset) override;
		virtual void* GetBufferData(Handle<Buffer> handle) override;
		virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) override;
		virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) override;
		virtual void MapBufferData(Handle<Buffer> buffer, intptr_t offset, intptr_t size) override;
		OpenGLBuffer* GetBuffer(Handle<Buffer> handle) const;

		// Framebuffers
		virtual Handle<FrameBuffer> CreateFrameBuffer(const FrameBufferDescriptor&& desc) override;
		virtual void DeleteFrameBuffer(Handle<FrameBuffer> handle) override;
		virtual void ResizeFrameBuffer(Handle<FrameBuffer> handle, uint32_t width, uint32_t height) override;
		OpenGLFrameBuffer* GetFrameBuffer(Handle<FrameBuffer> handle) const;

		// Shaders
		virtual Handle<Shader> CreateShader(const ShaderDescriptor&& desc) override;
		virtual void RecompileShader(Handle<Shader> handle, const ShaderDescriptor&& desc) override;
		virtual void DeleteShader(Handle<Shader> handle) override;
		virtual uint64_t GetOrAddShaderVariant(Handle<Shader> handle, const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc) override;
		OpenGLShader* GetShader(Handle<Shader> handle) const;

		// BindGroups
		virtual Handle<BindGroup> CreateBindGroup(const BindGroupDescriptor&& desc) override;
		virtual void DeleteBindGroup(Handle<BindGroup> handle) override;
		virtual void UpdateBindGroup(Handle<BindGroup> handle);
		virtual uint64_t GetBindGroupHash(Handle<BindGroup> handle) override;
		OpenGLBindGroup* GetBindGroup(Handle<BindGroup> handle) const;

		// BindGroupsLayouts
		virtual Handle<BindGroupLayout> CreateBindGroupLayout(const BindGroupLayoutDescriptor&& desc) override;
		virtual void DeleteBindGroupLayout(Handle<BindGroupLayout> handle) override;
		OpenGLBindGroupLayout* GetBindGroupLayout(Handle<BindGroupLayout> handle) const;

		// RenderPass
		virtual Handle<RenderPass> CreateRenderPass(const RenderPassDescriptor&& desc) override;
		virtual void DeleteRenderPass(Handle<RenderPass> handle) override;
		OpenGLRenderPass* GetRenderPass(Handle<RenderPass> handle) const;

		// RenderPassLayouts
		virtual Handle<RenderPassLayout> CreateRenderPassLayout(const RenderPassLayoutDescriptor&& desc) override;
		virtual void DeleteRenderPassLayout(Handle<RenderPassLayout> handle) override;
		OpenGLRenderPassLayout* GetRenderPassLayout(Handle<RenderPassLayout> handle) const;

	private:
		Pool<OpenGLTexture, Texture> m_TexturePool;
		Pool<OpenGLBuffer, Buffer> m_BufferPool;
		Pool<OpenGLShader, Shader> m_ShaderPool;
		Pool<OpenGLFrameBuffer, FrameBuffer> m_FrameBufferPool;
		Pool<OpenGLBindGroup, BindGroup> m_BindGroupPool;
		Pool<OpenGLBindGroupLayout, BindGroupLayout> m_BindGroupLayoutPool;
		Pool<OpenGLRenderPass, RenderPass> m_RenderPassPool;
		Pool<OpenGLRenderPassLayout, RenderPassLayout> m_RenderPassLayoutPool;

	private:
		uint64_t CalculateBindGroupHash(const OpenGLBindGroup* bindGroup);
	};
}
