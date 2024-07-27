#pragma once

#include "Base.h"

namespace HBL2
{
	struct TextureData
	{
		const uint8_t* Data;
		int Width;
		int Height;
	};

	class TextureUtilities
	{
	public:
		TextureUtilities(const TextureUtilities&) = delete;

		static TextureUtilities& Get()
		{
			static TextureUtilities instance;
			return instance;
		}

		TextureData Load(const std::string& path, bool flip = false);		

	private:
		TextureUtilities() = default;
	};
}