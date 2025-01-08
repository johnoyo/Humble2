#include "TextureUtilities.h"

namespace HBL2
{
	stbi_uc* TextureUtilities::Load(const std::string& path, TextureSettings& settings)
	{
		HBL2_FUNC_PROFILE();

		int bits = 0;
		stbi_uc* pixels = nullptr;

		if (!path.empty())
		{
			if (settings.Flip)
			{
				stbi_set_flip_vertically_on_load(1);
			}
			pixels = stbi_load(path.c_str(), &settings.Width, &settings.Height, &bits, STBI_rgb_alpha);
			assert(pixels);
		}

		return pixels;
	}

	void TextureUtilities::LoadWhiteTexture()
	{
		WhiteTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "white-texture",
			.dimensions = { 1.0f, 1.0f, 1.0f },
			.usage = TextureUsage::SAMPLED,
			.aspect = TextureAspect::COLOR,
			.initialData = nullptr,
		});
	}
}
