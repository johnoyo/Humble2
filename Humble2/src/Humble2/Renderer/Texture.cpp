#include "Texture.h"

#include "Renderer2D.h"
#include "Platform/OpenGL/OpenGLTextureOld.h"

namespace HBL
{
	std::unordered_map<std::string, Texture*> Texture::s_TextureLib;

	Texture* Texture::Load(const std::string& path)
	{
		if (Exists(path))
			return Get(path);

		switch (HBL2::RenderCommand::GetAPI())
		{
		case HBL2::GraphicsAPI::OpenGL:
			return new OpenGLTexture(path);
		case HBL2::GraphicsAPI::Vulkan:
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