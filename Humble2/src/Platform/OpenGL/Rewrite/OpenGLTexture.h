#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct OpenGLTexture
	{
		OpenGLTexture() = default;
		OpenGLTexture(const TextureDescriptor& desc)
		{
			DebugName = desc.debugName;
			Dimensions = desc.dimensions;
			Data = desc.initialData;

			glGenTextures(1, &RendererId);
			glBindTexture(GL_TEXTURE_2D, RendererId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			if (Data == nullptr)
			{
				uint32_t whiteTexture = 0xffffffff;
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Dimensions.x, Dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &whiteTexture);
			}
			else
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Dimensions.x, Dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data);
			}
		}

		const char* DebugName = "";
		uint32_t RendererId = 0;
		glm::vec3 Dimensions = glm::vec3(0.0);
		uint32_t Format = 0;
		const unsigned char* Data = nullptr;
	};
}