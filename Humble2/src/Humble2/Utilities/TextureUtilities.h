#pragma once

#include "Base.h"
#include "Resources/Handle.h"
#include "Resources/Types.h"
#include "Resources/ResourceManager.h"

namespace HBL2
{
	enum class CompressionMethod
	{
		NONE,
		BASISU,
		ASTC,
	};

	enum class CompressionQuality
	{
		FASTEST,
		FAST,
		MEDIUM,
		THOROUGH,
		EXHAUSTIVE,
	};

	struct HBL2_API TextureSettings
	{
		int Width = 0;
		int Height = 0;
		bool Flip = false;
        bool Relaod = false;
        
        Format PixelFormat = Format::RGBA8_RGB;

		StaticArray<CompressionMethod, 4> PlatformCompressionMethod = { CompressionMethod::NONE, CompressionMethod::NONE, CompressionMethod::NONE, CompressionMethod::NONE };
		StaticArray<Format, 4> PlatformCompressionFormat = { Format::RGBA8_RGB, Format::RGBA8_RGB, Format::RGBA8_RGB, Format::RGBA8_RGB };
		StaticArray<CompressionQuality, 4> PlatformCompressionQuality = { CompressionQuality::MEDIUM, CompressionQuality::MEDIUM, CompressionQuality::MEDIUM, CompressionQuality::MEDIUM };
	};

	class HBL2_API TextureUtilities
	{
	public:
		TextureUtilities(const TextureUtilities&) = delete;

		static TextureUtilities& Get();

		void* Load(const std::string& path, TextureSettings& settings);
		bool Save(const std::filesystem::path& path, const Span<const std::byte>& bytes, bool flip = false);

		void CreateAssetMetadataFile(Handle<Asset> handle);
		void SerializeAssetMetadataFile(Handle<Asset> handle, const TextureSettings& settings);
		TextureSettings DeserializeAssetMetadataFile(Handle<Asset> handle);
		TextureSettings DeserializeAssetMetadataFile(Asset* asset);

		void LoadWhiteTexture();
		void DeleteWhiteTexture();

		Handle<Texture> WhiteTexture;

	private:
		TextureUtilities() = default;
	};
}
