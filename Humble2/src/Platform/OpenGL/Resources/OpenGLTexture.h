#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\OpenGL\OpenGLCommon.h"

#include "Utilities\Collections\Span.h"

namespace HBL2
{
	struct OpenGLTexture
	{
		OpenGLTexture() = default;
		OpenGLTexture(const TextureDescriptor&& desc)
		{
			DebugName = desc.debugName;
			Dimensions = desc.dimensions;
			Format = OpenGLUtils::FormatToGLenum(desc.format);
			MinFilter = OpenGLUtils::FilterToGLenum(desc.sampler.filter);
			MagFilter = OpenGLUtils::FilterToGLenum(desc.sampler.filter);
			WrapMode = OpenGLUtils::WrapToGLenum(desc.sampler.wrap);

			glGenTextures(1, &RendererId);
			glBindTexture(GL_TEXTURE_2D, RendererId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MinFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MagFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WrapMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, WrapMode);

			switch (desc.aspect)
			{
			case TextureAspect::COLOR:
				Type = GL_UNSIGNED_BYTE;
				InternalFormat = GL_RGBA;
				break;
			case TextureAspect::DEPTH:
				Type = GL_UNSIGNED_INT;
				InternalFormat = GL_DEPTH_COMPONENT;
				break;
			}

			if (desc.initialData == nullptr && Dimensions.x == 1 && Dimensions.y == 1)
			{
				uint32_t whiteTexture = 0xffffffff;
				glTexImage2D(GL_TEXTURE_2D, 0, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, 0, InternalFormat, Type, &whiteTexture);
			}
			else
			{
				glTexImage2D(GL_TEXTURE_2D, 0, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, 0, InternalFormat, Type, desc.initialData);
				stbi_image_free(desc.initialData);
			}

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		void Update(const Span<const std::byte>& bytes)
		{
			Destroy();

			glGenTextures(1, &RendererId);
			glBindTexture(GL_TEXTURE_2D, RendererId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MinFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MagFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WrapMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, WrapMode);

			glTexImage2D(GL_TEXTURE_2D, 0, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, 0, InternalFormat, Type, bytes.Data());

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		void Destroy()
		{
			glDeleteTextures(1, &RendererId);
		}

		const char* DebugName = "";
		GLuint RendererId = 0;
		glm::vec3 Dimensions = glm::vec3(0.0f);
		GLenum Type = UINT32_MAX;
		GLenum Format = UINT32_MAX;
		GLenum InternalFormat = UINT32_MAX;
		GLenum MinFilter = UINT32_MAX;
		GLenum MagFilter = UINT32_MAX;
		GLenum WrapMode = UINT32_MAX;
	};
}