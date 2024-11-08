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
				return GL_FLOAT;
			case VertexFormat::FLOAT32x2:
				return GL_FLOAT;
			case VertexFormat::FLOAT32x3:
				return GL_FLOAT;
			case VertexFormat::FLOAT32x4:
				return GL_FLOAT;
			case VertexFormat::UINT32:
				return GL_UNSIGNED_INT;
			case VertexFormat::UINT32x2:
				return GL_UNSIGNED_INT;
			case VertexFormat::UINT32x3:
				return GL_UNSIGNED_INT;
			case VertexFormat::UINT32x4:
				return GL_UNSIGNED_INT;
			case VertexFormat::INT32:
				return GL_INT;
			case VertexFormat::INT32x2:
				return GL_INT;
			case VertexFormat::INT32x3:
				return GL_INT;
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
			}

			return -1;
		}
	}
}
