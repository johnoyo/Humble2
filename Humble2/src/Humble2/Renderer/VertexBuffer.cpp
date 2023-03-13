#include "VertexBuffer.h"

#include "Renderer2D.h"
#include "../../Platform/OpenGL/OpenGLVertexBuffer.h"

namespace HBL
{
	VertexBuffer* VertexBuffer::Create(uint32_t size)
	{
		switch (Renderer2D::Get().GetAPI())
		{
		case GraphicsAPI::OpenGL:
			return new OpenGLVertexBuffer(size);
		case GraphicsAPI::Vulkan:
			HBL_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLVertexBuffer(size);
		case HBL::GraphicsAPI::None:
			HBL_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			return nullptr;
		}

		return nullptr;
	}

	Buffer* VertexBuffer::GetHandle()
	{
		return m_Buffer;
	}
}