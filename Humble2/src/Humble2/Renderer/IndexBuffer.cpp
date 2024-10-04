#include "IndexBuffer.h"

#include "Renderer2D.h"
#include "Platform/OpenGL/OpenGLIndexBuffer.h"

namespace HBL2
{
	IndexBuffer* IndexBuffer::Create(uint32_t size, bool generated)
	{
		switch (RenderCommand::GetAPI())
		{
		case GraphicsAPI::OPENGL:
			return new OpenGLIndexBuffer(size, generated);
		case GraphicsAPI::VULKAN:
			HBL2_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLIndexBuffer(size, generated);
		case GraphicsAPI::NONE:
			HBL2_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			return nullptr;
		}

		return nullptr;
	}
}