#include "FrameBuffer.h"

#include "Renderer2D.h"
#include "Platform/OpenGL/OpenGLFrameBuffer.h"

namespace HBL2
{
	FrameBuffer* FrameBuffer::Create(FrameBufferSpecification& spec)
	{
		switch (RenderCommand::GetAPI())
		{
		case GraphicsAPI::OpenGL:
			return new OpenGLFrameBuffer(spec);
		case GraphicsAPI::Vulkan:
			HBL2_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLFrameBuffer(spec);
		case GraphicsAPI::None:
			HBL2_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			return nullptr;
		}
		
		HBL2_CORE_FATAL("No GraphicsAPI specified.");
		exit(-1);
		return nullptr;
	}
}
