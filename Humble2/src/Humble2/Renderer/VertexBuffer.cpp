#include "VertexBuffer.h"

#include "Renderer2D.h"
#include "Platform/OpenGL/OpenGLVertexBuffer.h"

namespace HBL2
{
	VertexBuffer* VertexBuffer::Create(uint32_t size, VertexBufferLayout& layout)
	{
		switch (RenderCommand::GetAPI())
		{
		case GraphicsAPI::OpenGL:
			return new OpenGLVertexBuffer(size, layout);
		case GraphicsAPI::Vulkan:
			HBL2_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLVertexBuffer(size, layout);
		case HBL2::GraphicsAPI::None:
			HBL2_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			return nullptr;
		}

		return nullptr;
	}

	VertexBuffer* VertexBuffer::Create(Buffer* handle, uint32_t size, VertexBufferLayout& layout)
	{
		switch (RenderCommand::GetAPI())
		{
		case GraphicsAPI::OpenGL:
			return new OpenGLVertexBuffer(handle, size, layout);
		case GraphicsAPI::Vulkan:
			HBL2_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLVertexBuffer(handle, size, layout);
		case HBL2::GraphicsAPI::None:
			HBL2_CORE_FATAL("No GraphicsAPI specified.");
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