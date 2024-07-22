#include "VertexArray.h"

#include "Renderer2D.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace HBL2
{
	VertexArray* VertexArray::Create()
	{
		switch (RenderCommand::GetAPI())
		{
		case GraphicsAPI::OPENGL:
			return new OpenGLVertexArray();
		case GraphicsAPI::VULKAN:
			HBL2_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLVertexArray();
		case HBL2::GraphicsAPI::NONE:
			HBL2_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			return nullptr;
		}

		return nullptr;
	}
}
