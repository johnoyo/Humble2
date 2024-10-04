#include "OpenGLTextureOld.h"

namespace HBL
{
	OpenGLTexture::OpenGLTexture(const std::string& path) : m_Path(path)
	{
		int w = 0, h = 0, bits = 0;
		stbi_uc* pixels = nullptr;

		if (!path.empty())
		{
			stbi_set_flip_vertically_on_load(1);
			pixels = stbi_load(m_Path.c_str(), &w, &h, &bits, STBI_rgb_alpha);
			assert(pixels);
		}

		Texture::Add(m_Path, this);

		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		m_SlotIndex = OpenGLTexture::s_CurrentSlot++;

		HBL2_CORE_TRACE("m_TextureID ({0}): {1}", m_Path, m_TextureID);
		HBL2_CORE_TRACE("m_SlotIndex ({0}): {1}", m_Path, m_SlotIndex);

		if (!path.empty())
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
			stbi_image_free(pixels);
			m_Width = (float)w;
			m_Height = (float)h;
		}
		else
		{
			uint32_t color = 0xffffffff;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color);
			m_Width = 1.f;
			m_Height = 1.f;
		}
	}

	void OpenGLTexture::Bind()
	{
#ifdef EMSCRIPTEN
		glActiveTexture(m_SlotIndex + GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);
#else
		glBindTextureUnit(m_SlotIndex, m_TextureID);
#endif
	}

	void OpenGLTexture::UnBind()
	{
#ifdef EMSCRIPTEN
		glActiveTexture(m_SlotIndex + GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
#else
		glBindTextureUnit(m_SlotIndex, 0);
#endif
	}

	uint32_t OpenGLTexture::GetID()
	{
		return m_TextureID;
	}

	uint32_t OpenGLTexture::GetSlot()
	{
		return m_SlotIndex;
	}

	float OpenGLTexture::GetWidth()
	{
		return m_Width;
	}

	float OpenGLTexture::GetHeight()
	{
		return m_Height;
	}
}