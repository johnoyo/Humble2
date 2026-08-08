#include "TextureUtilities.h"

#include "Project/Project.h"

#include <stb_image/stb_image_write.h>
#include <yaml-cpp/yaml.h>

#include <ktx.h>
#include <ktx/lib/src/vkformat_enum.h>

namespace HBL2
{
	TextureUtilities& TextureUtilities::Get()
	{
		static TextureUtilities instance;
		return instance;
	}

	void* TextureUtilities::Load(const std::string& path, TextureSettings& settings)
	{
		HBL2_FUNC_PROFILE();

		int bits = 0;
		void* pixels = nullptr;

		if (!path.empty())
		{
			if (settings.Flip)
			{
				stbi_set_flip_vertically_on_load(1);
			}
			else
			{
				stbi_set_flip_vertically_on_load(0);
			}

			const auto& pathAsPath = std::filesystem::path(path);
            ktx_uint32_t ktxFormat = 0;

			if (pathAsPath.extension() == ".hdr")
			{
				pixels = stbi_loadf(path.c_str(), &settings.Width, &settings.Height, &bits, STBI_rgb_alpha);
				settings.PixelFormat = Format::RGBA32_FLOAT;
                
                settings.PixelDataSize = (ktx_size_t)settings.Width * (ktx_size_t)settings.Height * 4 * sizeof(float);
                ktxFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			else
			{
				pixels = stbi_load(path.c_str(), &settings.Width, &settings.Height, &bits, STBI_rgb_alpha);
				settings.PixelFormat = Format::RGBA8_RGB;
                
                settings.PixelDataSize = (ktx_size_t)settings.Width * (ktx_size_t)settings.Height * 4;
                ktxFormat = VK_FORMAT_R8G8B8A8_SRGB;
			}
            
            HBL2_CORE_ASSERT(pixels, "Failed to load pixels!");
            
            ktxTexture2* kTexture;

            const auto& cachedPath = std::filesystem::path("assets") / "cache" / "texture" / (pathAsPath.stem().string() + ".ktx2");
            const auto& workingDir = Project::GetProjectDirectory().parent_path();
            auto texturePath = workingDir / cachedPath;
            
            // Ensure parent path exists.
            std::error_code ec;
            if (!std::filesystem::exists(texturePath.parent_path(), ec))
            {
                std::filesystem::create_directories(texturePath.parent_path(), ec);
                
                if (!ec)
                {
                    HBL2_CORE_ERROR("Texture cache directory creation failed: {0}", ec.message());
                    return nullptr;
                }
            }
            
            if (!ec)
            {
                HBL2_CORE_ERROR("Texture cache file inspection failed: {0}", ec.message());
                return nullptr;
            }

            // Check for cache hit for texture.
            if (std::filesystem::exists(texturePath, ec) && !settings.Relaod)
            {
                // Serialize ktx2 texture data to disk.
                KTX_error_code result = ktxTexture2_CreateFromNamedFile(texturePath.string().c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &kTexture);
                
                if (result != KTX_SUCCESS)
                {
                    ktxTexture_Destroy(ktxTexture(kTexture));
                    
                    HBL2_CORE_ERROR("Creation of ktx2 texture file failed: {0}", ktxErrorString(result));
                    return nullptr;
                }
                
                if (!kTexture->isCompressed)
                {
                    // Compress ktx2 texture data.
                    ktxAstcParams params = {0};
                    params.structSize = sizeof(params);
                    params.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_6x6;
                    params.qualityLevel = KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM;
                    params.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
                    params.threadCount = std::thread::hardware_concurrency() - 2;
                    
                    settings.PixelFormat = Format::ASTC_6x6_SRGB;

                    KTX_error_code result = ktxTexture2_CompressAstcEx(kTexture, &params);
                    
                    if (result != KTX_SUCCESS)
                    {
                        ktxTexture_Destroy(ktxTexture(kTexture));
                        HBL2_CORE_ERROR("Compression of ktx2 texture file failed: {0}", ktxErrorString(result));
                    }
                    else
                    {
                        result = ktxTexture2_WriteToNamedFile(kTexture, texturePath.string().c_str());
                        
                        if (result != KTX_SUCCESS)
                        {
                            ktxTexture_Destroy(ktxTexture(kTexture));
                            HBL2_CORE_ERROR("Serialization of ktx2 texture file failed: {0}", ktxErrorString(result));
                        }
                    }
                }
                else
                {
                    settings.PixelFormat = Format::ASTC_6x6_SRGB;
                }
            }
            else
            {
                // Create empty ktx2 texture.
                ktxTextureCreateInfo info{};
                info.vkFormat = ktxFormat;
                info.baseWidth = settings.Width;
                info.baseHeight = settings.Height;
                info.baseDepth = 1;
                info.numDimensions = 2;
                info.numLevels = 1;
                info.numLayers = 1;
                info.numFaces = 1;
                info.isArray = KTX_FALSE;
                info.generateMipmaps = KTX_FALSE;
                
                KTX_error_code result = ktxTexture2_Create(&info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &kTexture);

                if (result != KTX_SUCCESS)
                {
                    HBL2_CORE_ERROR("Failed to create ktx2 texture image: {0}", ktxErrorString(result));
                    return nullptr;
                }
                
                // Upload texture pixel data to ktx2 texture.
                result = ktxTexture_SetImageFromMemory(
                    ktxTexture(kTexture),
                    0, // mip level
                    0, // array layer
                    0, // face or depth slice
                    (const ktx_uint8_t*)pixels,
                    settings.PixelDataSize);

                if (result != KTX_SUCCESS)
                {
                    ktxTexture_Destroy(ktxTexture(kTexture));
                    
                    HBL2_CORE_ERROR("Failed to set ktx2 texture image data: {0}", ktxErrorString(result));
                    return nullptr;
                }
                
                // Compress ktx2 texture data.
                ktxAstcParams params = {0};
                params.structSize = sizeof(params);
                params.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_6x6;
                params.qualityLevel = KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM;
                params.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
                params.threadCount = std::thread::hardware_concurrency() - 2;
                
                settings.PixelFormat = Format::ASTC_6x6_SRGB;

                result = ktxTexture2_CompressAstcEx(kTexture, &params);
                
                if (result != KTX_SUCCESS)
                {
                    ktxTexture_Destroy(ktxTexture(kTexture));
                    HBL2_CORE_ERROR("Compression of ktx2 texture file failed: {0}", ktxErrorString(result));
                }
                else
                {
                    result = ktxTexture2_WriteToNamedFile(kTexture, texturePath.string().c_str());
                    
                    if (result != KTX_SUCCESS)
                    {
                        ktxTexture_Destroy(ktxTexture(kTexture));
                        HBL2_CORE_ERROR("Serialization of ktx2 texture file failed: {0}", ktxErrorString(result));
                    }
                }
            }
            
            // Get and return raw pixel data.
            ktx_size_t imageOffset = 0;
            ktxTexture_GetImageOffset(ktxTexture(kTexture), 0, 0, 0, &imageOffset);
            ktx_size_t levelSize = ktxTexture_GetImageSize(ktxTexture(kTexture), 0);

            void* textureData = std::malloc(levelSize);
            std::memcpy(textureData, ktxTexture_GetData(ktxTexture(kTexture)) + imageOffset, levelSize);
            settings.PixelDataSize = levelSize;

            ktxTexture_Destroy(ktxTexture(kTexture));
            
            stbi_image_free(pixels);
            
            return textureData;
		}

		return pixels;
	}

	bool TextureUtilities::Save(const std::filesystem::path& path, const Span<const std::byte>& bytes, bool flip)
	{
		if (!std::filesystem::exists(path.parent_path()))
		{
			try
			{
				std::filesystem::create_directories(path.parent_path());
			}
			catch (std::exception& e)
			{
				HBL2_ERROR("Project directory creation failed: {0}", e.what());
			}
		}

		int width;
		int height;
		int channels;

		if (flip)
		{
			stbi_set_flip_vertically_on_load(1);
		}

		stbi_uc* pixels = stbi_load_from_memory((const stbi_uc*)bytes.Data(), (int)bytes.Size(), &width, &height, &channels, STBI_default);

		if (pixels == NULL)
		{
			HBL2_CORE_ERROR("Failed to write image data to {}.", path);
			return false;
		}

		int result = stbi_write_png(path.string().c_str(), width, height, channels, pixels, width * channels);

		if (!result)
		{
			HBL2_CORE_ERROR("Failed to write image data to {}.", path);
			return false;
		}

		return true;
	}

	void TextureUtilities::CreateAssetMetadataFile(Handle<Asset> handle)
	{
		if (!AssetManager::Instance->IsAssetValid(handle))
		{
			return;
		}

		Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
		const std::filesystem::path& filePath = HBL2::Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hbltexture";

		if (std::filesystem::exists(filePath))
		{
			return;
		}

		std::ofstream fout(filePath, std::ios_base::out);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Texture" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(handle)->UUID;
		out << YAML::Key << "Flip" << YAML::Value << false;
        
        out << YAML::BeginMap;
        out << YAML::Key << "Compression" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "Windows" << YAML::Value << (uint32_t)Format::RGBA8_RGB;
        out << YAML::Key << "MacOS" << YAML::Value << (uint32_t)Format::RGBA8_RGB;
        out << YAML::Key << "Linux" << YAML::Value << (uint32_t)Format::RGBA8_RGB;
        out << YAML::Key << "Web" << YAML::Value << (uint32_t)Format::RGBA8_RGB;
        out << YAML::EndMap;
        out << YAML::EndMap;
        
		out << YAML::EndMap;
		out << YAML::EndMap;
		fout << out.c_str();
		fout.close();
	}

	void TextureUtilities::UpdateAssetMetadataFile(Handle<Asset> handle, const TextureSettings& settings)
	{
		if (!AssetManager::Instance->IsAssetValid(handle))
		{
			return;
		}

		Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
		const std::filesystem::path& filePath = HBL2::Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hbltexture";

		std::ofstream fout(filePath, std::ios_base::out);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Texture" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(handle)->UUID;
		out << YAML::Key << "Flip" << YAML::Value << settings.Flip;
        
        out << YAML::BeginMap;
        out << YAML::Key << "Compression" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "Windows" << YAML::Value << (uint32_t)settings.WindowsCompressionFormat;
        out << YAML::Key << "MacOS" << YAML::Value << (uint32_t)settings.MacOSCompressionFormat;
        out << YAML::Key << "Linux" << YAML::Value << (uint32_t)settings.LinuxCompressionFormat;
        out << YAML::Key << "Web" << YAML::Value << (uint32_t)settings.WebCompressionFormat;
        out << YAML::EndMap;
        out << YAML::EndMap;
        
		out << YAML::EndMap;
		out << YAML::EndMap;
		fout << out.c_str();
		fout.close();
	}

	void TextureUtilities::LoadWhiteTexture()
	{
		WhiteTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "white-texture",
			.dimensions = { 1.0f, 1.0f, 1.0f },
            .dataSize = 1 * 1 * 4,
			.usage = { TextureUsage::SAMPLED, TextureUsage::COPY_DST },
			.aspect = TextureAspect::COLOR,
			.initialData = nullptr,
		});
	}

	void TextureUtilities::DeleteWhiteTexture()
	{
		ResourceManager::Instance->DeleteTexture(WhiteTexture);
	}
}
