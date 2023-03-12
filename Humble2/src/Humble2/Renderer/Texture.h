#pragma once

#include "Renderer2D.h"

#include <string>
#include <unordered_map>

namespace HBL
{
	class Texture
	{
	public:
		virtual ~Texture() = default;

		static Texture* Load(const std::string& path);
		static Texture* Get(const std::string& name);
		static Texture* Get(uint32_t index);
		static bool Exists(const std::string& name);
		static void Add(const std::string& path, Texture* texture);

		static void ForEach(const std::function<void(Texture*)>& func);

		virtual void Bind() = 0;
		virtual void UnBind() = 0;
	private:
		static std::unordered_map<std::string, Texture*> s_TextureLib;
	};
}