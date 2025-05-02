#include "OpenGLTexture.h"

namespace HBL2
{
	OpenGLTexture::OpenGLTexture(const TextureDescriptor&& desc)
	{
		DebugName = desc.debugName;
		Dimensions = desc.dimensions;
		TextureType = OpenGLUtils::TextureTypeToGLenum(desc.type);
		Format = OpenGLUtils::FormatToGLenum(desc.format);
		MinFilter = OpenGLUtils::FilterToGLenum(desc.sampler.filter);
		MagFilter = OpenGLUtils::FilterToGLenum(desc.sampler.filter);
		WrapMode = OpenGLUtils::WrapToGLenum(desc.sampler.wrap);

		glGenTextures(1, &RendererId);
		glBindTexture(TextureType, RendererId);
		glTexParameteri(TextureType, GL_TEXTURE_MIN_FILTER, MinFilter);
		glTexParameteri(TextureType, GL_TEXTURE_MAG_FILTER, MagFilter);
		glTexParameteri(TextureType, GL_TEXTURE_WRAP_S, WrapMode);
		glTexParameteri(TextureType, GL_TEXTURE_WRAP_T, WrapMode);

		switch (desc.format)
		{
		case Format::D16_FLOAT:
		case Format::D24_FLOAT:
		case Format::D32_FLOAT:
		case Format::RGBA16_FLOAT:
		case Format::RGB32_FLOAT:
		case Format::RGBA32_FLOAT:
		case Format::RG16_FLOAT:
			Type = GL_FLOAT;
			break;
		case Format::RGBA8_RGB:
		case Format::RGBA8_UNORM:
		case Format::BGRA8_UNORM:
			Type = GL_UNSIGNED_BYTE;
			break;
		case Format::R10G10B10A2_UNORM:
			Type = GL_UNSIGNED_INT_2_10_10_10_REV;
			break;
		}

		switch (desc.aspect)
		{
		case TextureAspect::COLOR:
			InternalFormat = GL_RGBA;
			break;
		case TextureAspect::DEPTH:
			InternalFormat = GL_DEPTH_COMPONENT;
			break;
		}

		if (desc.initialData == nullptr && Dimensions.x == 1 && Dimensions.y == 1)
		{
			uint32_t whiteTexture = 0xffffffff;
			glTexImage2D(TextureType, 0, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, 0, InternalFormat, Type, &whiteTexture);
		}
		else
		{
			if (TextureType == GL_TEXTURE_CUBE_MAP)
			{
				for (unsigned int i = 0; i < 6; ++i)
				{
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, 0, InternalFormat, Type, nullptr);
				}
			}
			else
			{
				glTexImage2D(TextureType, 0, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, 0, InternalFormat, Type, (const void*)(float*)desc.initialData);
			}

			stbi_image_free(desc.initialData);
		}

		glBindTexture(TextureType, 0);
	}

	void OpenGLTexture::Bind(uint32_t slot)
	{
		switch (TextureType)
		{
		case GL_TEXTURE_2D:
			glActiveTexture(slot + GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, RendererId);
			break;
		case GL_TEXTURE_CUBE_MAP:
			glBindImageTexture(slot, RendererId, 0, GL_TRUE, 0, GL_WRITE_ONLY, Format);
			break;
		}
	}

	void OpenGLTexture::Update(const Span<const std::byte>& bytes)
	{
		Destroy();

		glGenTextures(1, &RendererId);
		glBindTexture(TextureType, RendererId);
		glTexParameteri(TextureType, GL_TEXTURE_MIN_FILTER, MinFilter);
		glTexParameteri(TextureType, GL_TEXTURE_MAG_FILTER, MagFilter);
		glTexParameteri(TextureType, GL_TEXTURE_WRAP_S, WrapMode);
		glTexParameteri(TextureType, GL_TEXTURE_WRAP_T, WrapMode);

		if (TextureType == GL_TEXTURE_CUBE_MAP)
		{
			for (unsigned int i = 0; i < 6; ++i)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, 0, GL_RGB, Type, bytes.Data());
			}
		}
		else
		{
			glTexImage2D(TextureType, 0, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, 0, InternalFormat, Type, bytes.Data());
		}

		glBindTexture(TextureType, 0);
	}

	void OpenGLTexture::Destroy()
	{
		glDeleteTextures(1, &RendererId);
	}
}