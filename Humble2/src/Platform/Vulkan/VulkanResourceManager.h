#pragma once

#include "Resources\ResourceManager.h"

#include "Resources\Pool.h"
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

		virtual void Clean() override {}

		// Textures
		virtual Handle<Texture> CreateTexture(const TextureDescriptor&& desc) override
		{
			return m_TexturePool.Insert(VulkanTexture(std::forward<const TextureDescriptor>(desc)));
		}
		virtual void DeleteTexture(Handle<Texture> handle) override
		{
			m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
			{
				VulkanTexture* texture = GetTexture(handle);
				if (texture != nullptr)
				{
					texture->Destroy();
					m_TexturePool.Remove(handle);
				}
			});
		}
		virtual void UpdateTexture(Handle<Texture> handle, const Span<const std::byte>& bytes) override
		{
			VulkanTexture* texture = GetTexture(handle);
			if (texture != nullptr)
			{
				texture->Update(bytes);
			}
		}
		virtual void TransitionTextureLayout(Handle<Texture> handle, TextureLayout currentLayout, TextureLayout newLayout, PipelineStage srcStage, PipelineStage dstStage) override
		{
			VulkanTexture* texture = GetTexture(handle);
			if (texture != nullptr)
			{
				texture->TrasitionLayout(currentLayout, newLayout, srcStage, dstStage);
			}
		}
		VulkanTexture* GetTexture(Handle<Texture> handle) const
		{
			return m_TexturePool.Get(handle);
		}

		// Buffers
		virtual Handle<Buffer> CreateBuffer(const BufferDescriptor&& desc) override
		{
			return m_BufferPool.Insert(VulkanBuffer(std::forward<const BufferDescriptor>(desc)));
		}
		virtual void DeleteBuffer(Handle<Buffer> handle) override
		{
			m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
			{
				VulkanBuffer* buffer = GetBuffer(handle);
				if (buffer != nullptr)
				{
					buffer->Destroy();
					m_BufferPool.Remove(handle);
				}
			});
		}
		virtual void ReAllocateBuffer(Handle<Buffer> handle, uint32_t currentOffset) override
		{
			VulkanBuffer* buffer = GetBuffer(handle);
			buffer->ReAllocate(currentOffset);
		}
		virtual void* GetBufferData(Handle<Buffer> handle) override
		{
			return m_BufferPool.Get(handle)->Data;
		}
		virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) override
		{
			VulkanBuffer* vulkanBuffer = GetBuffer(buffer);
			vulkanBuffer->Data = newData;
		}
		virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) override
		{
			VulkanBindGroup* vulkanBindGroup = GetBindGroup(bindGroup);
			if (bufferIndex < vulkanBindGroup->Buffers.size())
			{
				SetBufferData(vulkanBindGroup->Buffers[bufferIndex].buffer, vulkanBindGroup->Buffers[bufferIndex].byteOffset, newData);
			}
		}
		VulkanBuffer* GetBuffer(Handle<Buffer> handle) const
		{
			return m_BufferPool.Get(handle);
		}

		// Framebuffers
		virtual Handle<FrameBuffer> CreateFrameBuffer(const FrameBufferDescriptor&& desc) override
		{
			return m_FrameBufferPool.Insert(VulkanFrameBuffer(std::forward<const FrameBufferDescriptor>(desc)));
		}
		virtual void DeleteFrameBuffer(Handle<FrameBuffer> handle) override
		{
			m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
			{
				VulkanFrameBuffer* frameBuffer = GetFrameBuffer(handle);
				if (frameBuffer != nullptr)
				{
					frameBuffer->Destroy();
					m_FrameBufferPool.Remove(handle);
				}
			});
		}
		virtual void ResizeFrameBuffer(Handle<FrameBuffer> handle, uint32_t width, uint32_t height) override
		{
			if (!handle.IsValid())
			{
				return;
			}

			VulkanFrameBuffer* frameBuffer = GetFrameBuffer(handle);
			frameBuffer->Resize(width, height);
		}
		VulkanFrameBuffer* GetFrameBuffer(Handle<FrameBuffer> handle) const
		{
			return m_FrameBufferPool.Get(handle);
		}

		// Shaders
		virtual Handle<Shader> CreateShader(const ShaderDescriptor&& desc) override
		{
			return m_ShaderPool.Insert(VulkanShader(std::forward<const ShaderDescriptor>(desc)));
		}
		virtual void DeleteShader(Handle<Shader> handle) override
		{
			m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
			{
				VulkanShader* shader = GetShader(handle);
				if (shader != nullptr)
				{
					shader->Destroy();
					m_ShaderPool.Remove(handle);
				}
			});
		}
		virtual void AddShaderVariant(Handle<Shader> handle, const ShaderDescriptor::RenderPipeline::Variant& variantDesc) override
		{
			VulkanShader* shader = GetShader(handle);
			if (shader != nullptr)
			{
				shader->GetOrCreateVariant(variantDesc);
			}
		}
		VulkanShader* GetShader(Handle<Shader> handle) const
		{
			return m_ShaderPool.Get(handle);
		}

		// BindGroups
		virtual Handle<BindGroup> CreateBindGroup(const BindGroupDescriptor&& desc) override
		{
			// Caching mechanism so that materials with the same resources, use the same bind group.
			uint16_t index = 0;

			for (const auto& bindGroup : m_BindGroupPool.GetDataPool())
			{
				uint64_t descriptorHash = ResourceManager::Instance->GetBindGroupHash(desc);

				uint64_t hash = CalculateBindGroupHash(&bindGroup);

				if (descriptorHash == hash)
				{
					return m_BindGroupPool.GetHandleFromIndex(index);
				}

				index++;
			}

			return m_BindGroupPool.Insert(VulkanBindGroup(std::forward<const BindGroupDescriptor>(desc)));
		}
		virtual void DeleteBindGroup(Handle<BindGroup> handle) override
		{
			m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
			{
				VulkanBindGroup* bindGroup = GetBindGroup(handle);
				if (bindGroup != nullptr)
				{
					bindGroup->Destroy();
					m_BindGroupPool.Remove(handle);
				}
			});
		}
		virtual uint64_t GetBindGroupHash(Handle<BindGroup> handle) override
		{
			return CalculateBindGroupHash(GetBindGroup(handle));
		}
		VulkanBindGroup* GetBindGroup(Handle<BindGroup> handle) const
		{
			return m_BindGroupPool.Get(handle);
		}

		// BindGroupsLayouts
		virtual Handle<BindGroupLayout> CreateBindGroupLayout(const BindGroupLayoutDescriptor&& desc) override
		{
			return m_BindGroupLayoutPool.Insert(VulkanBindGroupLayout(std::forward<const BindGroupLayoutDescriptor>(desc)));
		}
		virtual void DeleteBindGroupLayout(Handle<BindGroupLayout> handle) override
		{
			m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
			{
				VulkanBindGroupLayout* bindGroupLayout = GetBindGroupLayout(handle);
				if (bindGroupLayout != nullptr)
				{
					bindGroupLayout->Destroy();
					m_BindGroupLayoutPool.Remove(handle);
				}
			});
		}
		VulkanBindGroupLayout* GetBindGroupLayout(Handle<BindGroupLayout> handle) const
		{
			return m_BindGroupLayoutPool.Get(handle);
		}

		// RenderPass
		virtual Handle<RenderPass> CreateRenderPass(const RenderPassDescriptor&& desc) override
		{
			return m_RenderPassPool.Insert(VulkanRenderPass(std::forward<const RenderPassDescriptor>(desc)));
		}
		virtual void DeleteRenderPass(Handle<RenderPass> handle) override
		{
			m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
			{
				VulkanRenderPass* renderPass = GetRenderPass(handle);
				if (renderPass != nullptr)
				{
					renderPass->Destroy();
					m_RenderPassPool.Remove(handle);
				}
			});
		}
		VulkanRenderPass* GetRenderPass(Handle<RenderPass> handle) const
		{
			return m_RenderPassPool.Get(handle);
		}

		// RenderPassLayouts
		virtual Handle<RenderPassLayout> CreateRenderPassLayout(const RenderPassLayoutDescriptor&& desc) override
		{
			return m_RenderPassLayoutPool.Insert(VulkanRenderPassLayout(std::forward<const RenderPassLayoutDescriptor>(desc)));
		}
		virtual void DeleteRenderPassLayout(Handle<RenderPassLayout> handle) override
		{
			m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
			{
				m_RenderPassLayoutPool.Remove(handle);
			});
		}
		VulkanRenderPassLayout* GetRenderPassLayout(Handle<RenderPassLayout> handle) const
		{
			return m_RenderPassLayoutPool.Get(handle);
		}

	private:
		Pool<VulkanTexture, Texture> m_TexturePool;
		Pool<VulkanBuffer, Buffer> m_BufferPool;
		Pool<VulkanShader, Shader> m_ShaderPool;
		Pool<VulkanFrameBuffer, FrameBuffer> m_FrameBufferPool;
		Pool<VulkanBindGroup, BindGroup> m_BindGroupPool;
		Pool<VulkanBindGroupLayout, BindGroupLayout> m_BindGroupLayoutPool;
		Pool<VulkanRenderPass, RenderPass> m_RenderPassPool;
		Pool<VulkanRenderPassLayout, RenderPassLayout> m_RenderPassLayoutPool;

		friend class VulkanRenderer; // This is required for a hack to create the swapchain images in the VulkanRenderer

	private:
		uint64_t CalculateBindGroupHash(const VulkanBindGroup* bindGroup)
		{
			if (bindGroup == nullptr)
			{
				return 0;
			}

			uint64_t hash = 0;

			for (const auto& bufferEntry : bindGroup->Buffers)
			{
				hash += bufferEntry.buffer.HashKey();
				hash += bufferEntry.byteOffset;
				hash += bufferEntry.range;
			}

			for (const auto texture : bindGroup->Textures)
			{
				hash += texture.HashKey();
			}

			hash += bindGroup->BindGroupLayout.HashKey();

			return hash;
		}
	};
}
