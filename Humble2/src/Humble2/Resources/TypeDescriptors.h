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
			uint32_t filter = 347567;
			Compare compare = Compare::LESS_OR_EQUAL;
			Wrap wrap = Wrap::CLAMP;
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
		Handle<RenderPassLayout> renderPassLayout;
		Handle<Texture> depthTarget;
		std::initializer_list<Handle<Texture>> colorTargets;
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
		std::initializer_list<Handle<BindGroupLayout>> bindGroups;
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
				std::vector<Attribute> attributes; // TODO: Change to initializer list.
			};

			struct BlendState
			{
				BlendOperation colorOp = BlendOperation::ADD;
				BlendFactor srcColorFactor = BlendFactor::SRC_ALPHA;
				BlendFactor dstColorFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
				BlendOperation aplhaOp = BlendOperation::ADD;
				BlendFactor srcAlhpaFactor = BlendFactor::SRC_ALPHA;
				BlendFactor dstAlhpaFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
			};

			Compare depthTest = Compare::LESS;
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
			uint32_t colorTargets = 0;
		};

		std::initializer_list<SubPass> subPasses;
	};

	struct RenderPassDescriptor
	{
		struct ColorTarget
		{
			LoadOperation loadOp = LoadOperation::CLEAR;
			StoreOperation storeOp = StoreOperation::STORE;
			TextureLayout nextUsage = TextureLayout::SAMPLED;
			glm::vec4 clearColor = glm::vec4(0.0f);
		};

		struct DepthTarget
		{
			LoadOperation loadOp = LoadOperation::CLEAR;
			StoreOperation storeOp = StoreOperation::STORE;
			LoadOperation stencilLoadOp = LoadOperation::CLEAR;
			StoreOperation stencilStoreOp = StoreOperation::STORE;
			TextureLayout nextUsage = TextureLayout::SAMPLED;
			float clearZ = 0.0f;
			uint32_t clearStencil = 0;
		};

		const char* debugName;
		Handle<RenderPassLayoutDescriptor> layout;
		DepthTarget depthTarget;
		std::initializer_list<ColorTarget> colorTargets;
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
