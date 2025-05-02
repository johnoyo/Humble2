#include "OpenGLCommon.h"

namespace HBL2
{
	namespace OpenGLUtils
	{
		GLenum VertexFormatToGLenum(VertexFormat vertexFormat)
		{
			switch (vertexFormat)
			{
			case VertexFormat::FLOAT32:
			case VertexFormat::FLOAT32x2:
			case VertexFormat::FLOAT32x3:
			case VertexFormat::FLOAT32x4:
				return GL_FLOAT;
			case VertexFormat::UINT32:
			case VertexFormat::UINT32x2:
			case VertexFormat::UINT32x3:
			case VertexFormat::UINT32x4:
				return GL_UNSIGNED_INT;
			case VertexFormat::INT32:
			case VertexFormat::INT32x2:
			case VertexFormat::INT32x3:
			case VertexFormat::INT32x4:
				return GL_INT;
			}

			return 0;
		}

		uint32_t VertexFormatSize(VertexFormat vertexFormat)
		{
			switch (vertexFormat)
			{
			case VertexFormat::FLOAT32:
			case VertexFormat::UINT32:
			case VertexFormat::INT32:
				return 1;
			case VertexFormat::FLOAT32x2:
			case VertexFormat::UINT32x2:
			case VertexFormat::INT32x2:
				return 2;
			case VertexFormat::FLOAT32x3:
			case VertexFormat::UINT32x3:
			case VertexFormat::INT32x3:
				return 3;
			case VertexFormat::FLOAT32x4:
			case VertexFormat::UINT32x4:
			case VertexFormat::INT32x4:
				return 4;
			}

			return 0;
		}

		GLenum FilterToGLenum(Filter filter)
		{
			switch (filter)
			{
			case HBL2::Filter::NEAREST:
				return GL_NEAREST;
			case HBL2::Filter::LINEAR:
				return GL_LINEAR;
			case HBL2::Filter::CUBIC:
				return -1; // TODO: Fix!
			}

			return -1;
		}

		GLenum WrapToGLenum(Wrap wrap)
		{
			switch (wrap)
			{
			case HBL2::Wrap::REPEAT:
				return GL_REPEAT;
			case HBL2::Wrap::REPEAT_MIRRORED:
				return GL_MIRRORED_REPEAT;
			case HBL2::Wrap::CLAMP_TO_EDGE:
				return GL_CLAMP_TO_EDGE;
			case HBL2::Wrap::CLAMP_TO_BORDER:
				return GL_CLAMP_TO_BORDER;
			case HBL2::Wrap::MIRROR_CLAMP_TO_EDGE:
				return GL_CLAMP_TO_EDGE;
			}

			return -1;
		}

		GLenum CompareToGLenum(Compare compare)
		{
			switch (compare)
			{
			case HBL2::Compare::LESS:
				return GL_LESS;
			case HBL2::Compare::LESS_OR_EQUAL:
				return GL_LEQUAL;
			case HBL2::Compare::GREATER:
				return GL_GREATER;
			case HBL2::Compare::GREATER_OR_EQUAL:
				return GL_GEQUAL;
			case HBL2::Compare::EQUAL:
				return GL_EQUAL;
			case HBL2::Compare::NOT_EQUAL:
				return GL_NOTEQUAL;
			case HBL2::Compare::ALAWAYS:
				return GL_ALWAYS;
			case HBL2::Compare::NEVER:
				return GL_NEVER;
			}

			return -1;
		}

		GLenum TextureTypeToGLenum(TextureType textureType)
		{
			switch (textureType)
			{
			case HBL2::TextureType::D1:
				return GL_TEXTURE_2D;
			case HBL2::TextureType::D2:
				return GL_TEXTURE_2D;
			case HBL2::TextureType::D3:
				return GL_TEXTURE_3D;
			case HBL2::TextureType::CUBE:
				return GL_TEXTURE_CUBE_MAP;
			}

			return -1;
		}

		GLenum FormatToGLenum(Format format)
		{
			switch (format)
			{
			case Format::D16_FLOAT:
				return GL_DEPTH_COMPONENT16;
			case Format::D24_FLOAT:
				return GL_DEPTH_COMPONENT24;
			case Format::D32_FLOAT:
				return GL_DEPTH_COMPONENT32;
			case Format::RGBA8_RGB:
				return GL_RGBA8;
			case Format::RGBA8_UNORM:
				return GL_RGBA8;
			case Format::BGRA8_UNORM:
				return GL_RGBA8;
			case Format::RGBA16_FLOAT:
				return GL_RGBA16F;
			case Format::RGB32_FLOAT:
				return GL_RGB32F;
			case Format::RGBA32_FLOAT:
				return GL_RGBA32F;
			case Format::RG16_FLOAT:
				return GL_RG16F;
			case Format::R10G10B10A2_UNORM:
				return GL_RGB10_A2;
			}

			return -1;
		}
	}
}
