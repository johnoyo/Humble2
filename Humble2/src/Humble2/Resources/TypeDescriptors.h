#pragma once

#include "Renderer\Enums.h"
#include "Handle.h"

#include "Utilities\Collections\Span.h"

#include <glm\glm.hpp>

#include <vector>
#include <initializer_list>

namespace HBL2
{
	struct Texture;
	struct TextureView;
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
		glm::u32vec3 dimensions;
		uint32_t mips = 1;
		Format format = Format::RGBA8_RGB;
		Format internalFormat = Format::RGBA8_RGB;
		TextureUsage usage = TextureUsage::TEXTURE_BINDING;
		TextureType type = TextureType::D2;
		TextureAspect aspect = TextureAspect::COLOR;

		struct Sampler
		{
			Filter filter = Filter::NEAREST;
			Compare compare = Compare::LESS_OR_EQUAL;
			Wrap wrap = Wrap::REPEAT;
			bool compareEnable = false;
		};

		bool createSampler = true;
		Sampler sampler;
		stbi_uc* initialData = nullptr;
	};

	struct BufferDescriptor
	{
		const char* debugName;
		BufferUsage usage = BufferUsage::UNIFORM;
		BufferUsageHint usageHint = BufferUsageHint::STATIC;
		MemoryUsage memoryUsage = MemoryUsage::CPU_GPU;
		uint32_t byteSize = 0;
		void* initialData = nullptr;
	};

	struct FrameBufferDescriptor
	{
		const char* debugName;
		uint32_t width = 0;
		uint32_t height = 0;
		Handle<RenderPass> renderPass;
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
			uint32_t range = 0;
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
				std::vector<Attribute> attributes;
			};

			struct BlendState
			{
				BlendOperation colorOp = BlendOperation::ADD;
				BlendFactor srcColorFactor = BlendFactor::SRC_ALPHA;
				BlendFactor dstColorFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
				BlendOperation alphaOp = BlendOperation::ADD;
				BlendFactor srcAlphaFactor = BlendFactor::ONE;
				BlendFactor dstAlphaFactor = BlendFactor::ZERO;
				bool enabled = true;
			};

			struct DepthTest
			{
				bool enabled = true;
				bool writeEnabled = true;
				bool stencilEnabled = true;
				Compare depthTest = Compare::LESS;
			};

			BlendState blend;
			DepthTest depthTest;
			Topology topology = Topology::TRIANGLE_LIST;
			PolygonMode polygonMode = PolygonMode::FILL;
			CullMode cullMode = CullMode::NONE;
			FrontFace frontFace = FrontFace::CLOCKWISE;
			std::initializer_list<VertexBufferBinding> vertexBufferBindings;
		};
		RenderPipeline renderPipeline;
		Handle<RenderPass> renderPass;
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

		Span<const SubPass> subPasses;
	};

	struct RenderPassDescriptor
	{
		struct ColorTarget
		{
			LoadOperation loadOp = LoadOperation::CLEAR;
			StoreOperation storeOp = StoreOperation::STORE;
			TextureLayout prevUsage = TextureLayout::UNDEFINED;
			TextureLayout nextUsage = TextureLayout::UNDEFINED;
			glm::vec4 clearColor = glm::vec4(0.0f);
		};

		struct DepthTarget
		{
			LoadOperation loadOp = LoadOperation::CLEAR;
			StoreOperation storeOp = StoreOperation::STORE;
			LoadOperation stencilLoadOp = LoadOperation::CLEAR;
			StoreOperation stencilStoreOp = StoreOperation::STORE;
			TextureLayout prevUsage = TextureLayout::UNDEFINED;
			TextureLayout nextUsage = TextureLayout::UNDEFINED;
			float clearZ = 0.0f;
			uint32_t clearStencil = 0;
		};

		const char* debugName;
		Handle<RenderPassLayout> layout;
		DepthTarget depthTarget;
		Span<const ColorTarget> colorTargets;
	};

	struct SubMeshDescriptor
	{
		const char* debugName;
		uint32_t indexOffset;
		uint32_t indexCount;
		uint32_t vertexOffset;
		uint32_t vertexCount;
		glm::vec3 minVertex;
		glm::vec3 maxVertex;
	};

	struct MeshPartDescriptor
	{
		const char* debugName;
		std::vector<SubMeshDescriptor> subMeshes;
		Handle<Buffer> indexBuffer;
		std::vector<Handle<Buffer>> vertexBuffers;
	};

	struct MeshDescriptor
	{
		const char* debugName;
		std::vector<MeshPartDescriptor> meshes;
	};

	struct MaterialDescriptor
	{
		const char* debugName;
		Handle<Shader> shader;
		Handle<BindGroup> bindGroup;
	};
}
