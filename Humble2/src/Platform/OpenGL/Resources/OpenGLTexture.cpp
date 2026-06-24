#include "OpenGLTexture.h"

#include "Utilities/JobSystem.h"

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
		LayerCount = desc.layerCount;

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

		glGenTextures(1, &RendererId);
		glBindTexture(TextureType, RendererId);

		if (desc.initialData == nullptr && Dimensions.x == 1 && Dimensions.y == 1)
		{
			glTexStorage2D(TextureType, 1, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y);
			uint32_t whiteTexture = 0xffffffff;
			glTexSubImage2D(TextureType, 0, 0, 0, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, InternalFormat, Type, &whiteTexture);
		}
		else
		{
			if (TextureType == GL_TEXTURE_CUBE_MAP)
			{
				glTexStorage2D(TextureType, 1, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y);
			}
			else if (TextureType == GL_TEXTURE_2D_ARRAY)
			{
				glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, LayerCount);
			}
			else
			{
				glTexStorage2D(TextureType, 1, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y);
				if (desc.initialData != nullptr)
				{
					glTexSubImage2D(TextureType, 0, 0, 0, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, InternalFormat, Type, (const void*)(float*)desc.initialData);
				}
			}

			if (desc.initialData != nullptr)
			{
				stbi_image_free(desc.initialData);
			}
		}

		glBindTexture(TextureType, 0);

		CreateView(TextureType, Format, LayerCount);

		if (!JobSystem::Get().IsRenderThread())
		{
			glFlush();
		}
	}

	void OpenGLTexture::Bind(uint32_t slot)
	{
		switch (TextureType)
		{
		case GL_TEXTURE_2D:
			glActiveTexture(slot + GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, ViewRendererId);
			break;
		case GL_TEXTURE_2D_ARRAY:
			glBindImageTexture(slot, RendererId, 0, GL_TRUE, 0, GL_WRITE_ONLY, Format);
			break;
		case GL_TEXTURE_CUBE_MAP:
			glActiveTexture(slot + GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, ViewRendererId);
			break;
		}
	}

	void OpenGLTexture::Update(const Span<const std::byte>& bytes)
	{
		Destroy();

		glGenTextures(1, &RendererId);
		glBindTexture(TextureType, RendererId);

		if (TextureType == GL_TEXTURE_CUBE_MAP)
		{
			glTexStorage2D(TextureType, 1, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y);
			for (unsigned int i = 0; i < 6; ++i)
			{
				glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, GL_RGB, Type, bytes.Data());
			}
		}
		else if (TextureType == GL_TEXTURE_2D_ARRAY)
		{
			glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, LayerCount);
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, LayerCount, GL_RGB, Type, bytes.Data());
		}
		else
		{
			glTexStorage2D(TextureType, 1, Format, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y);
			glTexSubImage2D(TextureType, 0, 0, 0, (GLsizei)Dimensions.x, (GLsizei)Dimensions.y, InternalFormat, Type, bytes.Data());
		}

		glBindTexture(TextureType, 0);

		CreateView(TextureType, Format, LayerCount);
	}

	void OpenGLTexture::ChangeTextureView(const TextureViewDescriptor&& desc)
	{
		LayerCount = desc.layerCount;

		GLenum newTarget = OpenGLUtils::TextureTypeToGLenum(desc.type);
		GLenum newFormat = OpenGLUtils::FormatToGLenum(desc.format);

		CreateView(newTarget, newFormat, LayerCount);

		TextureType = newTarget;
	}

	void* OpenGLTexture::GetData()
	{
		return nullptr;
	}

	void OpenGLTexture::Destroy()
	{
		glDeleteTextures(1, &ViewRendererId);
		glDeleteTextures(1, &RendererId);

		ViewRendererId = 0;
		RendererId = 0;
	}

	void OpenGLTexture::CreateView(GLenum viewTarget, GLenum viewFormat, GLuint layerCount)
	{
		if (ViewRendererId != 0)
		{
			glDeleteTextures(1, &ViewRendererId);
		}

		glGenTextures(1, &ViewRendererId);
		glTextureView(ViewRendererId, viewTarget, RendererId, viewFormat, 0, 1, 0, layerCount);

		glFinish(); // NOTE: force driver to complete view creation before setting params

		glBindTexture(viewTarget, ViewRendererId);
		glTexParameteri(viewTarget, GL_TEXTURE_MIN_FILTER, MinFilter);
		glTexParameteri(viewTarget, GL_TEXTURE_MAG_FILTER, MagFilter);
		glTexParameteri(viewTarget, GL_TEXTURE_WRAP_S, WrapMode);
		glTexParameteri(viewTarget, GL_TEXTURE_WRAP_T, WrapMode);

		if (WrapMode == GL_CLAMP_TO_BORDER)
		{
			float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(viewTarget, GL_TEXTURE_BORDER_COLOR, borderColor);
		}

		glBindTexture(viewTarget, 0);
	}
}
