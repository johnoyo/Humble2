#include "VertexArray.h"

#include "Renderer2D.h"
#include "../../Platform/OpenGL/OpenGLVertexArray.h"

namespace HBL
{
	VertexArray* VertexArray::Create()
	{
		switch (Renderer2D::Get().GetAPI())
		{
		case GraphicsAPI::OpenGL:
			return new OpenGLVertexArray();
		case GraphicsAPI::Vulkan:
			HBL_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLVertexArray();
		case HBL::GraphicsAPI::None:
			HBL_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			return nullptr;
		}

		return nullptr;
	}
}
