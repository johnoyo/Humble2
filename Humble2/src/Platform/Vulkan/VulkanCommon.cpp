#define VMA_IMPLEMENTATION
#include "VulkanCommon.h"

namespace HBL2
{
	namespace VkUtils
	{
		VkFormat VertexFormatToVkFormat(VertexFormat vertexFormat)
		{
			switch (vertexFormat)
			{
			case HBL2::VertexFormat::FLOAT32:
				return VK_FORMAT_R32_SFLOAT;
			case HBL2::VertexFormat::FLOAT32x2:
				return VK_FORMAT_R32G32_SFLOAT;
			case HBL2::VertexFormat::FLOAT32x3:
				return VK_FORMAT_R32G32B32_SFLOAT;
			case HBL2::VertexFormat::FLOAT32x4:
				return VK_FORMAT_R32G32B32A32_SFLOAT;
			case HBL2::VertexFormat::INT32:
				return VK_FORMAT_R32_SINT;
			case HBL2::VertexFormat::INT32x2:
				return VK_FORMAT_R32G32_SINT;
			case HBL2::VertexFormat::INT32x3:
				return VK_FORMAT_R32G32B32_SINT;
			case HBL2::VertexFormat::INT32x4:
				return VK_FORMAT_R32G32B32A32_SINT;
			case HBL2::VertexFormat::UINT32:
				return VK_FORMAT_R32_UINT;
			case HBL2::VertexFormat::UINT32x2:
				return VK_FORMAT_R32G32_UINT;
			case HBL2::VertexFormat::UINT32x3:
				return VK_FORMAT_R32G32B32_UINT;
			case HBL2::VertexFormat::UINT32x4:
				return VK_FORMAT_R32G32B32A32_UINT;
			}

			return VK_FORMAT_MAX_ENUM;
		}

		VkBlendOp BlendOperationToVkBlendOp(BlendOperation blendOperation)
		{
			switch (blendOperation)
			{
			case HBL2::BlendOperation::ADD:
				return VK_BLEND_OP_ADD;
			case HBL2::BlendOperation::MUL:
				return VK_BLEND_OP_MULTIPLY_EXT;
			case HBL2::BlendOperation::SUB:
				return VK_BLEND_OP_SUBTRACT;
			}

			return VK_BLEND_OP_MAX_ENUM;
		}

		VkBlendFactor BlendFactorToVkBlendFactor(BlendFactor blendFactor)
		{
			switch (blendFactor)
			{
			case HBL2::BlendFactor::SRC_ALPHA:
				return VK_BLEND_FACTOR_SRC_ALPHA;
			case HBL2::BlendFactor::ONE_MINUS_SRC_ALPHA:
				return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			case HBL2::BlendFactor::ONE:
				return VK_BLEND_FACTOR_ONE;
			case HBL2::BlendFactor::ZERO:
				return VK_BLEND_FACTOR_ZERO;
			}

			return VK_BLEND_FACTOR_MAX_ENUM;
		}

		VkCompareOp CompareToVkCompareOp(Compare compare)
		{
			switch (compare)
			{
			case HBL2::Compare::ALAWAYS:
				return VK_COMPARE_OP_ALWAYS;
			case HBL2::Compare::NEVER:
				return VK_COMPARE_OP_NEVER;
			case HBL2::Compare::LESS:
				return VK_COMPARE_OP_LESS;
			case HBL2::Compare::LESS_OR_EQUAL:
				return VK_COMPARE_OP_LESS_OR_EQUAL;
			case HBL2::Compare::GREATER:
				return VK_COMPARE_OP_GREATER;
			case HBL2::Compare::GREATER_OR_EQUAL:
				return VK_COMPARE_OP_GREATER_OR_EQUAL;
			case HBL2::Compare::EQUAL:
				return VK_COMPARE_OP_EQUAL;
			case HBL2::Compare::NOT_EQUAL:
				return VK_COMPARE_OP_NOT_EQUAL;
			}

			return VK_COMPARE_OP_MAX_ENUM;
		}

		VkFilter FilterToVkFilter(Filter filter)
		{
			switch (filter)
			{
			case HBL2::Filter::NEAREST:
				return VK_FILTER_NEAREST;
			case HBL2::Filter::LINEAR:
				return VK_FILTER_LINEAR;
			case HBL2::Filter::CUBIC:
				return VK_FILTER_CUBIC_EXT;
			}

			return VK_FILTER_MAX_ENUM;
		}

		VkSamplerAddressMode WrapToVkSamplerAddressMode(Wrap wrap)
		{
			switch (wrap)
			{
			case HBL2::Wrap::REPEAT:
				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case HBL2::Wrap::REPEAT_MIRRORED:
				return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			case HBL2::Wrap::CLAMP_TO_EDGE:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case HBL2::Wrap::CLAMP_TO_BORDER:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			case HBL2::Wrap::MIRROR_CLAMP_TO_EDGE:
				return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
			}

			return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
		}

		VkPrimitiveTopology TopologyToVkPrimitiveTopology(Topology topology)
		{
			switch (topology)
			{
			case HBL2::Topology::POINT_LIST:
				return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			case HBL2::Topology::LINE_LIST:
				return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			case HBL2::Topology::LINE_STRIP:
				return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
			case HBL2::Topology::TRIANGLE_LIST:
				return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			case HBL2::Topology::TRIANGLE_STRIP:
				return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			case HBL2::Topology::TRIANGLE_FAN:
				return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
			case HBL2::Topology::PATCH_LIST:
				return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			}

			return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
		}

		VkPolygonMode PolygonModeToVkPolygonMode(PolygonMode polygonMode)
		{
			switch (polygonMode)
			{
			case HBL2::PolygonMode::FILL:
				return VK_POLYGON_MODE_FILL;
			case HBL2::PolygonMode::LINE:
				return VK_POLYGON_MODE_LINE;
			case HBL2::PolygonMode::POINT:
				return VK_POLYGON_MODE_POINT;
			}

			return VK_POLYGON_MODE_MAX_ENUM;
		}

		VkCullModeFlags CullModeToVkCullModeFlags(CullMode cullMode)
		{
			switch (cullMode)
			{
			case HBL2::CullMode::NONE:
				return VK_CULL_MODE_NONE;
			case HBL2::CullMode::FRONT_BIT:
				return VK_CULL_MODE_FRONT_BIT;
			case HBL2::CullMode::BACK_BIT:
				return VK_CULL_MODE_BACK_BIT;
			case HBL2::CullMode::FRONT_AND_BACK:
				return VK_CULL_MODE_FRONT_AND_BACK;
			}

			return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
		}

		VkFrontFace FrontFaceToVkFrontFace(FrontFace frontFace)
		{
			switch (frontFace)
			{
			case HBL2::FrontFace::COUNTER_CLOCKWISE:
				return VK_FRONT_FACE_COUNTER_CLOCKWISE;
			case HBL2::FrontFace::CLOCKWISE:
				return VK_FRONT_FACE_CLOCKWISE;
			}

			return VK_FRONT_FACE_MAX_ENUM;
		}

		VkImageType TextureTypeToVkImageType(TextureType textureType)
		{
			switch (textureType)
			{
			case HBL2::TextureType::D1:
				return VK_IMAGE_TYPE_1D;
			case HBL2::TextureType::D2:
				return VK_IMAGE_TYPE_2D;
			case HBL2::TextureType::D3:
				return VK_IMAGE_TYPE_3D;
			default:
				break;
			}

			return VK_IMAGE_TYPE_MAX_ENUM;
		}

		VkImageViewType TextureTypeToVkVkImageViewType(TextureType textureType)
		{
			switch (textureType)
			{
			case HBL2::TextureType::D1:
				return VK_IMAGE_VIEW_TYPE_1D;
			case HBL2::TextureType::D2:
				return VK_IMAGE_VIEW_TYPE_2D;
			case HBL2::TextureType::D3:
				return VK_IMAGE_VIEW_TYPE_3D;
			default:
				break;
			}

			return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		}

		VkFormat FormatToVkFormat(Format format)
		{
			switch (format)
			{
			case Format::D32_FLOAT:
				return VK_FORMAT_D32_SFLOAT;
			case Format::RGBA8_RGB:
				return VK_FORMAT_R8G8B8A8_SRGB;
			case Format::RGBA8_UNORM:
				return VK_FORMAT_R8G8B8A8_UNORM;
			case Format::BGRA8_UNORM:
				return VK_FORMAT_B8G8R8A8_UNORM;
			}

			return VK_FORMAT_MAX_ENUM;
		}

		VkImageAspectFlags TextureAspectToVkImageAspectFlags(TextureAspect textureAspect)
		{
			switch (textureAspect)
			{
			case TextureAspect::NONE:
				return VK_IMAGE_ASPECT_NONE;
			case TextureAspect::COLOR:
				return VK_IMAGE_ASPECT_COLOR_BIT;
			case TextureAspect::DEPTH:
				return VK_IMAGE_ASPECT_DEPTH_BIT;
			case TextureAspect::STENCIL:
				return VK_IMAGE_ASPECT_STENCIL_BIT;
			}

			return VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
		}

		VkImageUsageFlags TextureUsageToVkImageUsageFlags(TextureUsage textureUsage)
		{
			switch (textureUsage)
			{
			case HBL2::TextureUsage::COPY_SRC:
				return VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			case HBL2::TextureUsage::COPY_DST:
				return VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			case HBL2::TextureUsage::TEXTURE_BINDING:
				return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT; // TODO: Investigate if its the corrent enum
			case HBL2::TextureUsage::STORAGE_BINDING:
				return VK_IMAGE_USAGE_STORAGE_BIT;
			case HBL2::TextureUsage::RENDER_ATTACHMENT:
				return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			case HBL2::TextureUsage::SAMPLED:
				return VK_IMAGE_USAGE_SAMPLED_BIT;
			case HBL2::TextureUsage::DEPTH_STENCIL:
				return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			default:
				break;
			}

			return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
		}

		VkImageLayout TextureLayoutToVkImageLayout(TextureLayout textureLayout)
		{
			switch (textureLayout)
			{
			case HBL2::TextureLayout::UNDEFINED:
				return VK_IMAGE_LAYOUT_UNDEFINED;
			case HBL2::TextureLayout::COPY_SRC:
				return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			case HBL2::TextureLayout::COPY_DST:
				return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			case HBL2::TextureLayout::RENDER_ATTACHMENT:
				return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			case HBL2::TextureLayout::SHADER_READ_ONLY:
				return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case HBL2::TextureLayout::DEPTH_STENCIL:
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			case HBL2::TextureLayout::PRESENT:
				return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			}

			return VK_IMAGE_LAYOUT_MAX_ENUM;
		}

		VkAttachmentLoadOp LoadOperationToVkAttachmentLoadOp(LoadOperation loadOperation)
		{
			switch (loadOperation)
			{
			case HBL2::LoadOperation::CLEAR:
				return VK_ATTACHMENT_LOAD_OP_CLEAR;
			case HBL2::LoadOperation::LOAD:
				return VK_ATTACHMENT_LOAD_OP_LOAD;
			case HBL2::LoadOperation::DONT_CARE:
				return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			}

			return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
		}

		VkAttachmentStoreOp StoreOperationVkAttachmentStoreOp(StoreOperation storeOperation)
		{
			switch (storeOperation)
			{
			case HBL2::StoreOperation::STORE:
				return VK_ATTACHMENT_STORE_OP_STORE;
			case HBL2::StoreOperation::DONT_CARE:
				return VK_ATTACHMENT_STORE_OP_DONT_CARE;
			}

			return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
		}

		VmaMemoryUsage MemoryUsageToVmaMemoryUsage(MemoryUsage memoryUsage)
		{
			switch (memoryUsage)
			{
			case HBL2::MemoryUsage::CPU_ONLY:
				return VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY;
			case HBL2::MemoryUsage::GPU_ONLY:
				return VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
			case HBL2::MemoryUsage::GPU_CPU:
				return VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_TO_CPU;
			case HBL2::MemoryUsage::CPU_GPU:
				return VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
			}

			return VMA_MEMORY_USAGE_MAX_ENUM;
		}

		VkBufferUsageFlags BufferUsageToVkBufferUsageFlags(BufferUsage bufferUsage)
		{
			switch (bufferUsage)
			{
			case HBL2::BufferUsage::MAP_READ:
				break;
			case HBL2::BufferUsage::MAP_WRITE:
				break;
			case HBL2::BufferUsage::COPY_SRC:
				return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			case HBL2::BufferUsage::COPY_DST:
				return VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			case HBL2::BufferUsage::INDEX:
				return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			case HBL2::BufferUsage::VERTEX:
				return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			case HBL2::BufferUsage::UNIFORM:
				return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			case HBL2::BufferUsage::STORAGE:
				return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			case HBL2::BufferUsage::INDIRECT:
				return VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			case HBL2::BufferUsage::QUERY_RESOLVE:
				break;
			}

			return VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
		}
	}
}