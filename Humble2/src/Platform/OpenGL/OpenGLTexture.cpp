#include "OpenGLTexture.h"

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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		m_SlotIndex = OpenGLTexture::s_CurrentSlot++;

		HBL_CORE_TRACE("m_TextureID ({0}): {1}", m_Path, m_TextureID);
		HBL_CORE_TRACE("m_SlotIndex ({0}): {1}", m_Path, m_SlotIndex);

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
		switch (m_SlotIndex)
		{
		case 0:
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_TextureID);
			break;
		case 1:
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, m_TextureID);
			break;
		case 2:
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, m_TextureID);
			break;
		case 3:
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, m_TextureID);
			break;
		case 4:
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, m_TextureID);
			break;
		case 5:
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, m_TextureID);
			break;
		case 6:
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D, m_TextureID);
			break;
		case 7:
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, m_TextureID);
			break;
		}
#else
		glBindTextureUnit(m_SlotIndex, m_TextureID);
#endif
	}

	void OpenGLTexture::UnBind()
	{
#ifdef EMSCRIPTEN
		switch (m_SlotIndex)
		{
		case 0:
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, 0);
			break;
		case 1:
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, 0);
			break;
		case 2:
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, 0);
			break;
		case 3:
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, 0);
			break;
		case 4:
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, 0);
			break;
		case 5:
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, 0);
			break;
		case 6:
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D, 0);
			break;
		case 7:
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, 0);
			break;
		}
#else
		glBindTextureUnit(m_SlotIndex, 0);
#endif
	}

	uint32_t OpenGLTexture::GetTextureID()
	{
		return m_TextureID;
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