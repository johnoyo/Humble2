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
	}
}
