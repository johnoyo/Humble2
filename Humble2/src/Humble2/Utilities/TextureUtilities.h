#pragma once

#include "Base.h"
#include "Resources\Handle.h"
#include "Resources\Types.h"
#include "Resources\ResourceManager.h"

namespace HBL2
{
	struct HBL2_API TextureSettings
	{
		int Width = 0;
		int Height = 0;
		bool Flip = false;
		Format PixelFormat = Format::RGBA8_RGB;
	};

	class HBL2_API TextureUtilities
	{
	public:
		TextureUtilities(const TextureUtilities&) = delete;

		static TextureUtilities& Get();

		void* Load(const std::string& path, TextureSettings& settings);
		bool Save(const std::filesystem::path& path, const Span<const std::byte>& bytes, bool flip = false);

		void CreateAssetMetadataFile(Handle<Asset> handle);
		void UpdateAssetMetadataFile(Handle<Asset> handle, bool flip = false);

		void LoadWhiteTexture();
		void DeleteWhiteTexture();

		Handle<Texture> WhiteTexture;

	private:
		TextureUtilities() = default;
	};
}