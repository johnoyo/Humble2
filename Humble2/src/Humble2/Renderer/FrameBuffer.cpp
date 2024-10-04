#include "FrameBuffer.h"

#include "Renderer2D.h"
#include "Platform/OpenGL/OpenGLFrameBufferOld.h"

namespace HBL
{
	FrameBuffer* FrameBuffer::Create(FrameBufferSpecification& spec)
	{
		switch (HBL2::RenderCommand::GetAPI())
		{
		case HBL2::GraphicsAPI::OPENGL:
			return new OpenGLFrameBuffer(spec);
		case HBL2::GraphicsAPI::VULKAN:
			HBL2_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLFrameBuffer(spec);
		case HBL2::GraphicsAPI::NONE:
			HBL2_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			return nullptr;
		}
		
		HBL2_CORE_FATAL("No GraphicsAPI specified.");
		exit(-1);
		return nullptr;
	}
}
