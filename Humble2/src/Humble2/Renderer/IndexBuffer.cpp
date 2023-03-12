#include "IndexBuffer.h"

#include "../../Platform/OpenGL/OpenGLIndexBuffer.h"

namespace HBL
{
	IndexBuffer* IndexBuffer::Create(uint32_t size)
	{
		switch (Renderer2D::Get().GetAPI())
		{
		case GraphicsAPI::OpenGL:
			return new OpenGLIndexBuffer(size);
		case GraphicsAPI::Vulkan:
			HBL_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLIndexBuffer(size);
		case HBL::GraphicsAPI::None:
			HBL_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			return nullptr;
		}

		return nullptr;
	}
}