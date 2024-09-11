#pragma once

#include "Renderer\Rewrite\Enums.h"
#include "Handle.h"

#include <glm\glm.hpp>

#include <vector>
#include <initializer_list>

namespace HBL2
{
	struct Texture;
	struct Buffer;
	struct Shader;
	struct Framebuffer;
	struct BindGroup;
	struct BindGroupLayout;
	struct RenderPass;
	struct RenderPassLayout;

	struct TextureDescriptor
	{
		const char* debugName;
		glm::vec3 dimensions;
		uint32_t mips = 1;
		uint32_t format = 347567;
		uint32_t internalFormat = 347567;
		TextureUsage usage = TextureUsage::TEXTURE_BINDING;

		struct Sampler
		{
			uint32_t compare = 347567;
		};

		const unsigned char* initialData = nullptr;
	};

	struct BufferDescriptor
	{
		const char* debugName;
		BufferUsage usage = BufferUsage::UNIFORM;
		BufferUsageHint usageHint = BufferUsageHint::STATIC;
		Memory memory = Memory::GPU_CPU;
		uint32_t byteSize = 0;
		void* initialData = nullptr;
	};

	struct FrameBufferDescriptor
	{
		const char* debugName;
		uint32_t width = 0;
		uint32_t height = 0;
		Handle<Texture> depthTarget;
		Handle<RenderPassLayout> renderPassLayout;
	};

	struct BindGroupLayoutDescriptor
	{
		const char* debugName;
		struct TextureBinding
		{
			uint32_t slot = 0;
			ShaderStage visibility = ShaderStage::VERTEX;
		};
		std::initializer_list<TextureBinding> textureBindings;

		struct BufferBinding
		{
			uint32_t slot = 0;
			ShaderStage visibility = ShaderStage::VERTEX;
			BufferBindingType type = BufferBindingType::UNIFORM;
		};
		std::initializer_list<BufferBinding> bufferBindings;
	};

	struct BindGroupDescriptor
	{
		const char* debugName;
		Handle<BindGroupLayout> layout;
		std::initializer_list<Handle<Texture>> textures;
		struct BufferEntry
		{
			Handle<Buffer> buffer;
			uint32_t byteOffset = 0;
		};
		std::initializer_list<BufferEntry> buffers;
	};

	struct ShaderDescriptor
	{
		const char* debugName;
		struct ShaderStage
		{
			std::vector<uint32_t> code;
			const char* entryPoint;
		};
		ShaderStage VS;
		ShaderStage FS;
		std::initializer_list<BindGroupDescriptor> bindGroups;
		struct RenderPipeline
		{
			struct VertexBufferBinding
			{
				struct Attribute
				{
					uint32_t byteOffset = 0;
					VertexFormat format = VertexFormat::FLOAT32;
				};

				uint32_t byteStride = 12;
				std::initializer_list<Attribute> attributes;
			};

			struct BlendState
			{
				uint32_t colorOp = 347567;
				uint32_t srcColorFactor = 347567;
				uint32_t dstColorFactor = 347567;
				uint32_t aplhaOp = 347567;
				uint32_t srcAlhpaFactor = 347567;
				uint32_t dstAlhpaFactor = 347567;
			};

			uint32_t depthTest = 347567;
			BlendState blend;
			std::initializer_list<VertexBufferBinding> vertexBufferBindings;
		};
		RenderPipeline renderPipeline;
		Handle<RenderPassLayout> renderPassLayout;
	};

	struct RenderPassLayoutDescriptor
	{
		const char* debugName;
		Format depthTargetFormat = Format::D32_FLOAT;

		struct SubPass
		{
			bool depthTarget = false;
		};

		std::initializer_list<SubPass> subPasses;
	};

	struct RenderPassDescriptor
	{
		const char* debugName;
		Handle<RenderPassLayoutDescriptor> layout;

		struct DepthTarget
		{
			TextureLayout nextUsage = TextureLayout::SAMPLED;
			float clearZ = 0.0f;
		};

		DepthTarget depthTarget;
	};

	struct MeshDescriptor
	{
		const char* debugName;
		uint32_t indexOffset;
		uint32_t indexCount;
		uint32_t vertexOffset;
		uint32_t vertexCount;
		Handle<Buffer> indexBuffer;
		std::initializer_list<Handle<Buffer>> vertexBuffers;
	};

	struct MaterialDescriptor
	{
		const char* debugName;
		Handle<Shader> shader;
		Handle<BindGroup> bindGroup;
	};
}
