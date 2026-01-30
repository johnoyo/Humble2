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
				bool enabled = false;
			};

			struct DepthTest
			{
				bool enabled = true;
				bool writeEnabled = false;
				bool stencilEnabled = true;
				Compare depthTest = Compare::LESS_OR_EQUAL;
			};

			using packed_size = uint64_t;

			struct PackedVariant
			{
				// Raster
				packed_size topology : 3 = (packed_size)Topology::TRIANGLE_LIST;
				packed_size polygonMode : 2 = (packed_size)PolygonMode::FILL;
				packed_size cullMode : 2 = (packed_size)CullMode::BACK;
				packed_size frontFace : 1 = (packed_size)FrontFace::CLOCKWISE;

				// Blend
				packed_size blendEnabled : 1 = 0; // false
				packed_size colorOutput : 1 = 1; // true
				packed_size colorOp : 3 = (packed_size)BlendOperation::ADD;
				packed_size alphaOp : 3 = (packed_size)BlendOperation::ADD;
				packed_size srcColorFactor : 2 = (packed_size)BlendFactor::SRC_ALPHA;
				packed_size dstColorFactor : 2 = (packed_size)BlendFactor::ONE_MINUS_SRC_ALPHA;
				packed_size srcAlphaFactor : 2 = (packed_size)BlendFactor::ONE;
				packed_size dstAlphaFactor : 2 = (packed_size)BlendFactor::ZERO;

				// Depth
				packed_size depthEnabled : 1 = 1; // true
				packed_size depthWrite : 1 = 0; // false
				packed_size stencilEnabled : 1 = 1; // true
				packed_size depthCompare : 3 = (packed_size)Compare::LESS_OR_EQUAL;

				packed_size _padding : 34 = 0;

				constexpr uint64_t Key() const noexcept { return std::bit_cast<uint64_t>(*this); }

				friend constexpr bool operator<(const PackedVariant& a, const PackedVariant& b) noexcept
				{
					return a.Key() < b.Key();
				}

				friend constexpr bool operator==(const PackedVariant& a, const PackedVariant& b) noexcept
				{
					return a.Key() == b.Key();
				}
			};

			static_assert(sizeof(PackedVariant) == sizeof(packed_size));
			static_assert(std::is_trivially_copyable_v<PackedVariant>);

			std::initializer_list<VertexBufferBinding> vertexBufferBindings;

			Span<const PackedVariant> variants;
		};
		RenderPipeline renderPipeline;
		Handle<RenderPass> renderPass;
	};

	static inline const ShaderDescriptor::RenderPipeline::PackedVariant g_NullVariant = std::bit_cast<ShaderDescriptor::RenderPipeline::PackedVariant>(uint64_t{ 0 });

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

	struct MeshDescriptorEx
	{
		const char* debugName;
		std::vector<MeshPartDescriptor> meshes;
	};

	struct MeshDescriptorSettings
	{
		bool recalculateNormals = false;
		bool recalculateBounds = false;
	};

	struct MeshDescriptor
	{
		const char* debugName;
		Span<const float> vertices;
		Span<const uint32_t> indeces;
		MeshDescriptorSettings settings = {};
	};

	struct MaterialDescriptor
	{
		const char* debugName;
		Handle<Shader> shader;
		Handle<BindGroup> bindGroup;
	};
}
