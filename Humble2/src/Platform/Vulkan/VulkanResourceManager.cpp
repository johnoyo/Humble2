#include "VulkanResourceManager.h"

namespace HBL2
{
	void VulkanResourceManager::Initialize(const ResourceManagerSpecification& spec)
	{
		m_Spec = spec;

		InternalInitialize();

		m_TexturePool.Initialize(m_Spec.Textures);
		m_BufferSplitPool.Initialize(m_Spec.Buffers);
		m_ShaderSplitPool.Initialize(m_Spec.Shaders);
		m_FrameBufferPool.Initialize(m_Spec.FrameBuffers);
		m_BindGroupSplitPool.Initialize(m_Spec.BindGroups);
		m_BindGroupLayoutPool.Initialize(m_Spec.BindGroupLayouts);
		m_RenderPassPool.Initialize(m_Spec.RenderPass);
		m_RenderPassLayoutPool.Initialize(m_Spec.RenderPassLayouts);
	}
	const ResourceManagerSpecification VulkanResourceManager::GetUsageStats()
	{
		ResourceManagerSpecification currentSpec =
		{
			.Textures = m_TexturePool.FreeSlotCount(),
			.Buffers = m_BufferSplitPool.FreeSlotCount(),
			.FrameBuffers = m_FrameBufferPool.FreeSlotCount(),
			.Shaders = m_ShaderSplitPool.FreeSlotCount(),
			.BindGroups = m_BindGroupSplitPool.FreeSlotCount(),
			.BindGroupLayouts = m_BindGroupLayoutPool.FreeSlotCount(),
			.RenderPass = m_RenderPassPool.FreeSlotCount(),
			.RenderPassLayouts = m_RenderPassLayoutPool.FreeSlotCount(),
			.Meshes = m_MeshPool.FreeSlotCount(),
			.Materials = m_MaterialPool.FreeSlotCount(),
			.Scenes = m_ScenePool.FreeSlotCount(),
			.Scripts = m_ScriptPool.FreeSlotCount(),
			.Sounds = m_SoundPool.FreeSlotCount(),
			.Prefabs = m_PrefabPool.FreeSlotCount(),
		};

		return currentSpec;
	}
	void VulkanResourceManager::Clean()
	{
	}

	// Textures
	Handle<Texture> VulkanResourceManager::CreateTexture(const TextureDescriptor&& desc)
	{
		return m_TexturePool.Insert(std::forward<const TextureDescriptor>(desc));
	}
	void VulkanResourceManager::DeleteTexture(Handle<Texture> handle)
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
	void VulkanResourceManager::UpdateTexture(Handle<Texture> handle, const Span<const std::byte>& bytes)
	{
		VulkanTexture* texture = GetTexture(handle);
		if (texture != nullptr)
		{
			texture->Update(bytes);
		}
	}
	void VulkanResourceManager::TransitionTextureLayout(CommandBuffer* commandBuffer, Handle<Texture> handle, TextureLayout currentLayout, TextureLayout newLayout, Handle<BindGroup> bindGroupHandle)
	{
		VulkanTexture* texture = GetTexture(handle);
		if (texture != nullptr)
		{
			VulkanBindGroup bindGroup = GetBindGroup(bindGroupHandle);
			texture->TrasitionLayout((VulkanCommandBuffer*)commandBuffer, currentLayout, newLayout, &bindGroup);
		}
	}
	glm::vec3 VulkanResourceManager::GetTextureDimensions(Handle<Texture> handle)
	{
		VulkanTexture* texture = GetTexture(handle);
		if (texture != nullptr)
		{
			return { texture->Extent.width, texture->Extent.height, texture->Extent.depth };
		}

		return { 0.f, 0.f, 0.f };
	}
	void* VulkanResourceManager::GetTextureData(Handle<Texture> handle)
	{
		return nullptr;
	}
	VulkanTexture* VulkanResourceManager::GetTexture(Handle<Texture> handle) const
	{
		return m_TexturePool.Get(handle);
	}

	// Buffers
	Handle<Buffer> VulkanResourceManager::CreateBuffer(const BufferDescriptor&& desc)
	{
		VulkanBuffer buffer;
		Handle<Buffer> bufferHandle = m_BufferSplitPool.Insert(&buffer.Hot, &buffer.Cold);
		buffer.Initialize(std::forward<const BufferDescriptor>(desc));
		return bufferHandle;
	}
	void VulkanResourceManager::DeleteBuffer(Handle<Buffer> handle)
	{
		m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
		{
			VulkanBuffer buffer = GetBuffer(handle);
			if (buffer.IsValid())
			{
				buffer.Destroy();
				m_BufferSplitPool.Remove(handle);
			}
		});
	}
	void VulkanResourceManager::ReAllocateBuffer(Handle<Buffer> handle, uint32_t currentOffset)
	{
		VulkanBuffer buffer = GetBuffer(handle);
		buffer.ReAllocate(currentOffset);
	}
	void* VulkanResourceManager::GetBufferData(Handle<Buffer> handle)
	{
		return GetBufferHot(handle)->Data;
	}
	void VulkanResourceManager::SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData)
	{
		VulkanBufferHot* vulkanBuffer = GetBufferHot(buffer);
		vulkanBuffer->Data = (void*)((char*)newData + offset);
	}
	void VulkanResourceManager::SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData)
	{
		VulkanBindGroupCold* vulkanBindGroupCold = GetBindGroupCold(bindGroup);
		if (bufferIndex < vulkanBindGroupCold->Buffers.size())
		{
			SetBufferData(vulkanBindGroupCold->Buffers[bufferIndex].buffer, vulkanBindGroupCold->Buffers[bufferIndex].byteOffset, newData);
		}
	}
	void VulkanResourceManager::MapBufferData(Handle<Buffer> buffer, intptr_t offset, intptr_t size)
	{
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

		VulkanBufferHot* vulkanBuffer = GetBufferHot(buffer);

		if (vulkanBuffer == nullptr)
		{
			return;
		}

		void* data;
		vmaMapMemory(renderer->GetAllocator(), vulkanBuffer->Allocation, &data);
		memcpy((void*)((char*)data + offset), (void*)((char*)vulkanBuffer->Data + offset), size);
		vmaUnmapMemory(renderer->GetAllocator(), vulkanBuffer->Allocation);
	}
	VulkanBuffer VulkanResourceManager::GetBuffer(Handle<Buffer> handle) const
	{
		VulkanBuffer buffer;
		if (m_BufferSplitPool.Get(handle, &buffer.Hot, &buffer.Cold))
		{
			return buffer;
		}

		return {};
	}
	VulkanBufferHot* VulkanResourceManager::GetBufferHot(Handle<Buffer> handle) const
	{
		return m_BufferSplitPool.GetHot(handle);
	}
	VulkanBufferCold* VulkanResourceManager::GetBufferCold(Handle<Buffer> handle) const
	{
		return m_BufferSplitPool.GetCold(handle);
	}

	// Framebuffers
	Handle<FrameBuffer> VulkanResourceManager::CreateFrameBuffer(const FrameBufferDescriptor&& desc)
	{
		return m_FrameBufferPool.Insert(std::forward<const FrameBufferDescriptor>(desc));
	}
	void VulkanResourceManager::DeleteFrameBuffer(Handle<FrameBuffer> handle)
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
	void VulkanResourceManager::ResizeFrameBuffer(Handle<FrameBuffer> handle, uint32_t width, uint32_t height)
	{
		if (!handle.IsValid())
		{
			return;
		}

		VulkanFrameBuffer* frameBuffer = GetFrameBuffer(handle);
		frameBuffer->Resize(width, height);
	}
	VulkanFrameBuffer* VulkanResourceManager::GetFrameBuffer(Handle<FrameBuffer> handle) const
	{
		return m_FrameBufferPool.Get(handle);
	}

	// Shaders
	Handle<Shader> VulkanResourceManager::CreateShader(const ShaderDescriptor&& desc)
	{
		VulkanShader shader;
		Handle<Shader> shaderHandle = m_ShaderSplitPool.Insert(&shader.Hot, &shader.Cold);
		shader.Initialize(std::forward<const ShaderDescriptor>(desc));
		return shaderHandle;
	}
	void VulkanResourceManager::RecompileShader(Handle<Shader> handle, const ShaderDescriptor&& desc)
	{
		VulkanShader shader = GetShader(handle);
		shader.Recompile(std::forward<const ShaderDescriptor>(desc), true);

		m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
		{
			VulkanShader shader = GetShader(handle);
			shader.DestroyOld();
		});
	}
	void VulkanResourceManager::DeleteShader(Handle<Shader> handle)
	{
		m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
		{
			VulkanShader shader = GetShader(handle);
			if (shader.IsValid())
			{
				shader.Destroy();
				m_ShaderSplitPool.Remove(handle);
			}
		});
	}
	uint64_t VulkanResourceManager::GetOrAddShaderVariant(Handle<Shader> handle, const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc)
	{
		VulkanShader shader = GetShader(handle);
		return (uint64_t)shader.GetOrCreateVariant(variantDesc);
	}
	VulkanShader VulkanResourceManager::GetShader(Handle<Shader> handle) const
	{
		VulkanShader shader;
		if (m_ShaderSplitPool.Get(handle, &shader.Hot, &shader.Cold))
		{
			return shader;
		}

		return {};
	}
	VulkanShaderHot* VulkanResourceManager::GetShaderHot(Handle<Shader> handle) const
	{
		return m_ShaderSplitPool.GetHot(handle);
	}
	VulkanShaderCold* VulkanResourceManager::GetShaderCold(Handle<Shader> handle) const
	{
		return m_ShaderSplitPool.GetCold(handle);
	}

	// BindGroups
	Handle<BindGroup> VulkanResourceManager::CreateBindGroup(const BindGroupDescriptor&& desc)
	{
		// Caching mechanism so that materials with the same resources, use the same bind group.
		uint16_t index = 0;
		uint64_t descriptorHash = ResourceManager::Instance->GetBindGroupHash(desc);

		for (const auto& bindGroup : m_BindGroupSplitPool.GetDataColdPool())
		{
			uint64_t hash = CalculateBindGroupHash(&bindGroup);

			if (descriptorHash == hash)
			{
				VulkanBindGroupCold* mutBindGroup = (VulkanBindGroupCold*)&bindGroup;
				if (mutBindGroup->TryAddRef())
				{
					return m_BindGroupSplitPool.GetHandleFromIndex(index);
				}

				break;
			}

			index++;
		}

		VulkanBindGroup bindgroup;
		Handle<BindGroup> bg = m_BindGroupSplitPool.Insert(&bindgroup.Hot, &bindgroup.Cold);
		bindgroup.Initialize(std::forward<const BindGroupDescriptor>(desc));

		// Increase ref count of bindgroup.
		bindgroup.Cold->TryAddRef();

		return bg;
	}
	void VulkanResourceManager::DeleteBindGroup(Handle<BindGroup> handle)
	{
		VulkanBindGroupCold* bindGroupCold = GetBindGroupCold(handle);
		if (bindGroupCold->ReleaseRefAndMaybeDelete())
		{
			m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
			{
				VulkanBindGroupCold* bindGroupCold = GetBindGroupCold(handle);
				if (bindGroupCold != nullptr)
				{
					bindGroupCold->Destroy();
					m_BindGroupSplitPool.Remove(handle);
				}
			});
		}
	}
	void VulkanResourceManager::UpdateBindGroup(Handle<BindGroup> handle)
	{
		VulkanBindGroup bindGroup = GetBindGroup(handle);
		bindGroup.Update();
	}
	uint64_t VulkanResourceManager::GetBindGroupHash(Handle<BindGroup> handle)
	{
		return CalculateBindGroupHash(GetBindGroupCold(handle));
	}
	VulkanBindGroup VulkanResourceManager::GetBindGroup(Handle<BindGroup> handle) const
	{
		VulkanBindGroup bindGroup;
		if (m_BindGroupSplitPool.Get(handle, &bindGroup.Hot, &bindGroup.Cold))
		{
			return bindGroup;
		}

		return {};
	}
	VulkanBindGroupHot* VulkanResourceManager::GetBindGroupHot(Handle<BindGroup> handle) const
	{
		return m_BindGroupSplitPool.GetHot(handle);
	}
	VulkanBindGroupCold* VulkanResourceManager::GetBindGroupCold(Handle<BindGroup> handle) const
	{
		return m_BindGroupSplitPool.GetCold(handle);
	}
	uint64_t VulkanResourceManager::CalculateBindGroupHash(const VulkanBindGroupCold* bindGroupCold)
	{
		if (bindGroupCold == nullptr)
		{
			return 0;
		}

		uint64_t hash = 0;

		for (const auto& bufferEntry : bindGroupCold->Buffers)
		{
			hash += bufferEntry.buffer.HashKey() + typeid(Buffer).hash_code();
			hash += bufferEntry.byteOffset;
			hash += bufferEntry.range;
		}

		for (const auto texture : bindGroupCold->Textures)
		{
			hash += texture.HashKey() + typeid(Texture).hash_code();
		}

		hash += bindGroupCold->BindGroupLayout.HashKey() + typeid(BindGroupLayout).hash_code();

		return hash;
	}

	// BindGroupsLayouts
	Handle<BindGroupLayout> VulkanResourceManager::CreateBindGroupLayout(const BindGroupLayoutDescriptor&& desc)
	{
		return m_BindGroupLayoutPool.Insert(std::forward<const BindGroupLayoutDescriptor>(desc));
	}
	void VulkanResourceManager::DeleteBindGroupLayout(Handle<BindGroupLayout> handle)
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
	VulkanBindGroupLayout* VulkanResourceManager::GetBindGroupLayout(Handle<BindGroupLayout> handle) const
	{
		return m_BindGroupLayoutPool.Get(handle);
	}
	
	// RenderPass
	Handle<RenderPass> VulkanResourceManager::CreateRenderPass(const RenderPassDescriptor&& desc)
	{
		return m_RenderPassPool.Insert(std::forward<const RenderPassDescriptor>(desc));
	}
	void VulkanResourceManager::DeleteRenderPass(Handle<RenderPass> handle)
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
	VulkanRenderPass* VulkanResourceManager::GetRenderPass(Handle<RenderPass> handle) const
	{
		return m_RenderPassPool.Get(handle);
	}

	// RenderPassLayouts
	Handle<RenderPassLayout> VulkanResourceManager::CreateRenderPassLayout(const RenderPassLayoutDescriptor&& desc)
	{
		return m_RenderPassLayoutPool.Insert(std::forward<const RenderPassLayoutDescriptor>(desc));
	}
	void VulkanResourceManager::DeleteRenderPassLayout(Handle<RenderPassLayout> handle)
	{
		m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
		{
			m_RenderPassLayoutPool.Remove(handle);
		});
	}
	VulkanRenderPassLayout* VulkanResourceManager::GetRenderPassLayout(Handle<RenderPassLayout> handle) const
	{
		return m_RenderPassLayoutPool.Get(handle);
	}
}

