#pragma once

#include "Renderer/Texture.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

#include "stb_image/stb_image.h"

namespace HBL
{
	class OpenGLTexture final : public Texture
	{
	public:
		OpenGLTexture(const std::string& path);
		virtual ~OpenGLTexture() {}

		virtual void Bind() override;
		virtual void UnBind() override;

		virtual uint32_t GetID() override;
		virtual uint32_t GetSlot() override;

		virtual float GetWidth() override;
		virtual float GetHeight() override;
	private:
		std::string m_Path;
		uint32_t m_SlotIndex;
		uint32_t m_TextureID;
		float m_Width;
		float m_Height;
		static inline uint32_t s_CurrentSlot = 0;
	};
}