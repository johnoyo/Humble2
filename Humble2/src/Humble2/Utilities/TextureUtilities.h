#pragma once

#include "Base.h"

namespace HBL2
{
	struct TextureSettings
	{
		bool Flip = false;
	};

	struct TextureData
	{
		const uint8_t* Data;
		TextureSettings Settings;
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

		TextureData Load(const std::string& path, const TextureSettings& settings);		

	private:
		TextureUtilities() = default;
	};
}