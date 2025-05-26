#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\OpenGL\OpenGLCommon.h"

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct OpenGLFrameBuffer
	{
		OpenGLFrameBuffer() = default;
		OpenGLFrameBuffer(const FrameBufferDescriptor&& desc)
			: DebugName(desc.debugName), Width(desc.width), Height(desc.height), ColorTargets(desc.colorTargets), DepthTarget(desc.depthTarget)
		{
			Create();
		}

		void Create();
		void Resize(uint32_t width, uint32_t height);
		void Destroy();

		const char* DebugName = "";
		GLuint RendererId = 0;
		uint32_t Width = 0;
		uint32_t Height = 0;

		std::vector<Handle<Texture>> ColorTargets;
		Handle<Texture> DepthTarget;
	};
}