#include "Texture.h"

#include "Renderer2D.h"
#include "../../Platform/OpenGL/OpenGLTexture.h"

namespace HBL2
{
	std::unordered_map<std::string, Texture*> Texture::s_TextureLib;

	Texture* Texture::Load(const std::string& path)
	{
		if (Exists(path))
			return Get(path);

		switch (Renderer2D::Get().GetAPI())
		{
		case GraphicsAPI::OpenGL:
			return new OpenGLTexture(path);
		case GraphicsAPI::Vulkan:
			HBL2_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLTexture(path);
		case HBL2::GraphicsAPI::None:
			HBL2_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
		}

		return nullptr;
	}

	Texture* Texture::Get(const std::string& name)
	{
		if (!Exists(name))
			return Load(name);

		return s_TextureLib[name];
	}

	Texture* Texture::Get(uint32_t index)
	{
		int i = 0;
		for (auto& item : s_TextureLib)
		{
			if (i++ == index)
				return item.second;
		}

		return nullptr;
	}

	bool Texture::Exists(const std::string& name)
	{
		return s_TextureLib.find(name) != s_TextureLib.end();
	}

	void Texture::Add(const std::string& path, Texture* texture)
	{
		s_TextureLib[path] = texture;
	}

	void Texture::ForEach(const std::function<void(Texture*)>& func)
	{
		for (auto& item : s_TextureLib)
		{
			func(item.second);
		}
	}
}