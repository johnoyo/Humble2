#include "TextureUtilities.h"

namespace HBL2
{
    TextureData TextureUtilities::Load(const std::string& path, bool flip)
	{
		int w = 0, h = 0, bits = 0;
		stbi_uc* pixels = nullptr;

		if (!path.empty())
		{
			if (flip)
			{
				stbi_set_flip_vertically_on_load(1);
			}
			pixels = stbi_load(path.c_str(), &w, &h, &bits, STBI_rgb_alpha);
			assert(pixels);
		}

		return { pixels, w, h };
	}
}
