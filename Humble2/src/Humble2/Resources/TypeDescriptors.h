#pragma once

#include "Renderer\Enums.h"
#include "Handle.h"

#include "Utilities\Collections\Span.h"
#include "Utilities\Collections\BitFlags.h"

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
	struct Material;

	struct TextureDescriptor
	{
		const char* debugName;
		glm::u32vec3 dimensions;
		uint32_t mips = 1;
		Format format = Format::RGBA8_RGB;
		Format internalFormat = Format::RGBA8_RGB;
		BitFlags<TextureUsage> usage = TextureUsage::TEXTURE_BINDING;
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
		TextureLayout initialLayout = TextureLayout::SHADER_READ_ONLY;
		void* initialData = nullptr;
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
			TextureBindingType type = TextureBindingType::IMAGE_SAMPLER;
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
		ShaderType type = ShaderType::RASTERIZATION;
		struct ShaderStage
		{
			Span<const uint32_t> code;
			const char* entryPoint;
		};
		ShaderStage VS;
		ShaderStage FS;
		ShaderStage CS;
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
				bool colorOutput = true;
				bool enabled = true;
			};

			struct DepthTest
			{
				bool enabled = true;
				bool writeEnabled = true;
				bool stencilEnabled = true;
				Compare depthTest = Compare::LESS;
			};

			struct Variant
			{
				UUID shaderHashKey = 0;
				BlendState blend{};
				DepthTest depthTest{};
				Topology topology = Topology::TRIANGLE_LIST;
				PolygonMode polygonMode = PolygonMode::FILL;
				CullMode cullMode = CullMode::BACK;
				FrontFace frontFace = FrontFace::COUNTER_CLOCKWISE;

				inline bool operator==(const Variant& other) const
				{
					return blend.colorOp == other.blend.colorOp &&
						blend.srcColorFactor == other.blend.srcColorFactor &&
						blend.dstColorFactor == other.blend.dstColorFactor &&
						blend.alphaOp == other.blend.alphaOp &&
						blend.srcAlphaFactor == other.blend.srcAlphaFactor &&
						blend.dstAlphaFactor == other.blend.dstAlphaFactor &&
						blend.colorOutput == other.blend.colorOutput &&
						blend.enabled == other.blend.enabled &&

						depthTest.enabled == other.depthTest.enabled &&
						depthTest.writeEnabled == other.depthTest.writeEnabled &&
						depthTest.stencilEnabled == other.depthTest.stencilEnabled &&
						depthTest.depthTest == other.depthTest.depthTest &&

						topology == other.topology &&
						polygonMode == other.polygonMode &&
						cullMode == other.cullMode &&
						frontFace == other.frontFace &&

						shaderHashKey == other.shaderHashKey;
				}
			};

			std::initializer_list<VertexBufferBinding> vertexBufferBindings;

			Span<const Variant> variants;
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
			Format format = Format::BGRA8_UNORM;
			LoadOperation loadOp = LoadOperation::CLEAR;
			StoreOperation storeOp = StoreOperation::STORE;
			TextureLayout prevUsage = TextureLayout::UNDEFINED;
			TextureLayout nextUsage = TextureLayout::UNDEFINED;
			glm::vec4 clearColor = glm::vec4(0.25f, 0.25f, 0.25f, 1.0f);
		};

		struct DepthTarget
		{
			LoadOperation loadOp = LoadOperation::CLEAR;
			StoreOperation storeOp = StoreOperation::STORE;
			LoadOperation stencilLoadOp = LoadOperation::CLEAR;
			StoreOperation stencilStoreOp = StoreOperation::STORE;
			TextureLayout prevUsage = TextureLayout::UNDEFINED;
			TextureLayout nextUsage = TextureLayout::UNDEFINED;
			float clearZ = 1.0f;
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

		Handle<Material> embededMaterial;
	};

	struct MeshPartDescriptor
	{
		const char* debugName;
		std::vector<SubMeshDescriptor> subMeshes;
		Handle<Buffer> indexBuffer;
		std::vector<Handle<Buffer>> vertexBuffers;

		struct LocalTransform
		{
			glm::vec3 translation;
			glm::vec3 rotation;
			glm::vec3 scale;
		} importedLocalTransform;
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

namespace std
{
	template<>
	struct hash<HBL2::ShaderDescriptor::RenderPipeline::Variant>
	{
		size_t operator()(const HBL2::ShaderDescriptor::RenderPipeline::Variant& variantDesc) const
		{
			size_t h = 0;

			// Helper to combine hashes (standard approach)
			auto hash_combine = [](size_t& seed, size_t hash)
			{
				seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			};

			// Hash Variant
			const auto& v = variantDesc;

			// BlendState
			hash_combine(h, std::hash<int>()(static_cast<int>(v.blend.colorOp)));
			hash_combine(h, std::hash<int>()(static_cast<int>(v.blend.srcColorFactor)));
			hash_combine(h, std::hash<int>()(static_cast<int>(v.blend.dstColorFactor)));
			hash_combine(h, std::hash<int>()(static_cast<int>(v.blend.alphaOp)));
			hash_combine(h, std::hash<int>()(static_cast<int>(v.blend.srcAlphaFactor)));
			hash_combine(h, std::hash<int>()(static_cast<int>(v.blend.dstAlphaFactor)));
			hash_combine(h, std::hash<bool>()(v.blend.colorOutput));
			hash_combine(h, std::hash<bool>()(v.blend.enabled));

			// DepthTest
			hash_combine(h, std::hash<bool>()(v.depthTest.enabled));
			hash_combine(h, std::hash<bool>()(v.depthTest.writeEnabled));
			hash_combine(h, std::hash<bool>()(v.depthTest.stencilEnabled));
			hash_combine(h, std::hash<int>()(static_cast<int>(v.depthTest.depthTest)));

			// Other Variant fields
			hash_combine(h, std::hash<int>()(static_cast<int>(v.topology)));
			hash_combine(h, std::hash<int>()(static_cast<int>(v.polygonMode)));
			hash_combine(h, std::hash<int>()(static_cast<int>(v.cullMode)));
			hash_combine(h, std::hash<int>()(static_cast<int>(v.frontFace)));

			// Shader hash key
			hash_combine(h, std::hash<HBL2::UUID>()(static_cast<HBL2::UUID>(v.shaderHashKey)));

			return h;
		}
	};
}
