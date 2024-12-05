#pragma once

#include "Base.h"
#include "Resources\Handle.h"
#include "Resources\TypeDescriptors.h"

#include "OpenGLBuffer.h"
#include "OpenGLTexture.h"
#include "OpenGLBindGroupLayout.h"

#include "Platform\OpenGL\OpenGLCommon.h"

namespace HBL2
{
	class OpenGLResourceManager;

	struct OpenGLBindGroup
	{
		OpenGLBindGroup() = default;
		OpenGLBindGroup(const BindGroupDescriptor&& desc);		

		void Set();
		void Destroy();

		const char* DebugName = "";
		std::vector<BindGroupDescriptor::BufferEntry> Buffers;
		std::vector<Handle<Texture>> Textures;
		Handle<BindGroupLayout> BindGroupLayout;
	};
}