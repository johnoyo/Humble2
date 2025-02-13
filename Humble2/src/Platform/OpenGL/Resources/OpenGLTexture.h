#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\OpenGL\OpenGLCommon.h"

namespace HBL2
{
	struct OpenGLTexture
	{
		OpenGLTexture() = default;
		OpenGLTexture(const TextureDescriptor&& desc)
		{
			DebugName = desc.debugName;
			Dimensions = desc.dimensions;
			Data = desc.initialData;

			glGenTextures(1, &RendererId);
			glBindTexture(GL_TEXTURE_2D, RendererId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, OpenGLUtils::FilterToGLenum(desc.sampler.filter));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, OpenGLUtils::FilterToGLenum(desc.sampler.filter));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, OpenGLUtils::WrapToGLenum(desc.sampler.wrap));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, OpenGLUtils::WrapToGLenum(desc.sampler.wrap));

			GLenum type;
			GLenum internalFormat;

			switch (desc.aspect)
			{
			case TextureAspect::COLOR:
				type = GL_UNSIGNED_BYTE;
				internalFormat = GL_RGBA;
				break;
			case TextureAspect::DEPTH:
				type = GL_UNSIGNED_INT;
				internalFormat = GL_DEPTH_COMPONENT;
				break;
			}

			if (Data == nullptr && Dimensions.x == 1 && Dimensions.y == 1)
			{
				uint32_t whiteTexture = 0xffffffff;
				glTexImage2D(GL_TEXTURE_2D, 0, OpenGLUtils::FormatToGLenum(desc.format), (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, 0, internalFormat, type, &whiteTexture);
			}
			else
			{
				glTexImage2D(GL_TEXTURE_2D, 0, OpenGLUtils::FormatToGLenum(desc.format), (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, 0, internalFormat, type, Data);
				stbi_image_free(Data);
			}

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		void Destroy()
		{
			glDeleteTextures(1, &RendererId);
		}

		const char* DebugName = "";
		GLuint RendererId = 0;
		glm::vec3 Dimensions = glm::vec3(0.0f);
		uint32_t Format = 0;
		stbi_uc* Data = nullptr;
	};
}