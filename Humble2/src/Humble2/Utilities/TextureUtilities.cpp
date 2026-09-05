#include "TextureUtilities.h"

#include "Project/Project.h"

#include "Platform/PlatformManager.h"

#include <stb_image/stb_image_write.h>
#include <yaml-cpp/yaml.h>

#include <ktx.h>
#include <ktx/lib/src/vkformat_enum.h>

namespace HBL2
{
    static uint16_t FloatToHalf(float f)
    {
        uint32_t bits;
        std::memcpy(&bits, &f, sizeof(bits));

        uint32_t sign = (bits >> 16) & 0x8000;
        int32_t exponent = ((bits >> 23) & 0xFF) - 127 + 15;
        uint32_t mantissa = bits & 0x7FFFFF;

        if (exponent <= 0)  return (uint16_t)sign;              // flush tiny values to zero
        if (exponent >= 31) return (uint16_t)(sign | 0x7BFF);   // clamp overflow instead of inf

        return (uint16_t)(sign | (exponent << 10) | (mantissa >> 13));
    }

    static ktx_pack_uastc_flags CompressionQualityToUASTC(CompressionQuality compressionQuality)
    {
        switch (compressionQuality)
        {
        case CompressionQuality::FASTEST:
            return KTX_PACK_UASTC_LEVEL_FASTEST;
        case CompressionQuality::FAST:
            return KTX_PACK_UASTC_LEVEL_FASTER;
        case CompressionQuality::MEDIUM:
            return KTX_PACK_UASTC_LEVEL_DEFAULT;
        case CompressionQuality::THOROUGH:
            return KTX_PACK_UASTC_LEVEL_SLOWER;
        case CompressionQuality::EXHAUSTIVE:
            return KTX_PACK_UASTC_LEVEL_VERYSLOW;
        }
    }

    static ktx_pack_astc_quality_levels CompressionQualityToASTC(CompressionQuality compressionQuality)
    {
        switch (compressionQuality)
        {
        case CompressionQuality::FASTEST:
            return KTX_PACK_ASTC_QUALITY_LEVEL_FASTEST;
        case CompressionQuality::FAST:
            return KTX_PACK_ASTC_QUALITY_LEVEL_FAST;
        case CompressionQuality::MEDIUM:
            return KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM;
        case CompressionQuality::THOROUGH:
            return KTX_PACK_ASTC_QUALITY_LEVEL_THOROUGH;
        case CompressionQuality::EXHAUSTIVE:
            return KTX_PACK_ASTC_QUALITY_LEVEL_EXHAUSTIVE;
        }
    }

    static ktx_transcode_fmt_e FormatToTranscodeFormat(Format format)
    {
        switch (format)
        {
        case Format::BC1_RGB_SRGB:
        case Format::BC1_RGB_UNORM:
            return KTX_TTF_BC1_RGB;
        case Format::BC3_SRGB:
        case Format::BC3_UNORM:
            return KTX_TTF_BC3_RGBA;
        case Format::BC6H_UF:
            return KTX_TTF_BC6HU;
        case Format::BC7_SRGB:
        case Format::BC7_UNORM:
            return KTX_TTF_BC7_RGBA;
        case Format::ASTC_4x4_SRGB:
        case Format::ASTC_4x4_UNORM:
            return KTX_TTF_ASTC_4x4_RGBA;
        }
    }

    static ktx_uint32_t FormatToASTCBlockFormat(Format format)
    {
        switch (format)
        {
        case Format::ASTC_4x4_SRGB:
        case Format::ASTC_4x4_UNORM:
            return KTX_PACK_ASTC_BLOCK_DIMENSION_4x4;
        case Format::ASTC_5x5_SRGB:
        case Format::ASTC_5x5_UNORM:
            return KTX_PACK_ASTC_BLOCK_DIMENSION_5x5;
        case Format::ASTC_6x6_SRGB:
        case Format::ASTC_6x6_UNORM:
            return KTX_PACK_ASTC_BLOCK_DIMENSION_6x6;
        case Format::ASTC_8x8_SRGB:
        case Format::ASTC_8x8_UNORM:
            return KTX_PACK_ASTC_BLOCK_DIMENSION_8x8;
        case Format::ASTC_10x10_SRGB:
        case Format::ASTC_10x10_UNORM:
            return KTX_PACK_ASTC_BLOCK_DIMENSION_10x10;
        case Format::ASTC_12x12_SRGB:
        case Format::ASTC_12x12_UNORM:
            return KTX_PACK_ASTC_BLOCK_DIMENSION_12x12;
        }
    }

    static bool CompressBasisU(ktxTexture2* kTexture, const std::filesystem::path& texturePath, bool isHDR, ktx_pack_uastc_flags qualityLevel, ktx_transcode_fmt_e outputFormat)
    {
        // Setup Basis Universal compression parameters
        ktxBasisParams params = { 0 };
        params.structSize = sizeof(params);
        params.codec = isHDR ? KTX_BASIS_CODEC_UASTC_HDR_4x4 : KTX_BASIS_CODEC_UASTC_LDR_4x4;
        params.uastcFlags = qualityLevel; // Quality level
        params.threadCount = std::thread::hardware_concurrency() - 2;

        // Compress texture
        KTX_error_code result = ktxTexture2_CompressBasisEx(kTexture, &params);

        if (result != KTX_SUCCESS)
        {
            HBL2_CORE_ERROR("Compression of ktx2 texture file failed: {0}", ktxErrorString(result));
            return false;
        }
        else
        {
            if (isHDR)
            {
                if (outputFormat == KTX_TTF_ASTC_4x4_RGBA)
                {
                    outputFormat = KTX_TTF_ASTC_HDR_4x4_RGBA;
                }

                // Always transcode for hdr.
                result = ktxTexture2_TranscodeBasis(kTexture, outputFormat, 0);
            }
            else
            {
                if (ktxTexture2_NeedsTranscoding(kTexture))
                {
                    result = ktxTexture2_TranscodeBasis(kTexture, outputFormat, 0);
                }
            }

            if (result != KTX_SUCCESS)
            {
                HBL2_CORE_ERROR("Transcode failed: {0}", ktxErrorString(result));
                return false;
            }
            else
            {
                result = ktxTexture2_WriteToNamedFile(kTexture, texturePath.string().c_str());

                if (result != KTX_SUCCESS)
                {
                    HBL2_CORE_ERROR("Serialization of ktx2 texture file failed: {0}", ktxErrorString(result));
                    return false;
                }
            }

            return true;
        }
    }

    static bool CompressAstc(ktxTexture2* kTexture, const std::filesystem::path& texturePath, bool isHDR, ktx_pack_astc_quality_levels qualityLevel, ktx_uint32_t outputFormat)
    {
        // Compress ktx2 texture data.
        ktxAstcParams params = { 0 };
        params.structSize = sizeof(params);
        params.blockDimension = outputFormat;
        params.qualityLevel = qualityLevel;
        params.mode = isHDR ? KTX_PACK_ASTC_ENCODER_MODE_HDR : KTX_PACK_ASTC_ENCODER_MODE_LDR;
        params.threadCount = std::thread::hardware_concurrency() - 2;

        KTX_error_code result = ktxTexture2_CompressAstcEx(kTexture, &params);

        if (result != KTX_SUCCESS)
        {
            HBL2_CORE_ERROR("Compression of ktx2 texture file failed: {0}", ktxErrorString(result));
            return false;
        }
        else
        {
            result = ktxTexture2_WriteToNamedFile(kTexture, texturePath.string().c_str());

            if (result != KTX_SUCCESS)
            {
                HBL2_CORE_ERROR("Serialization of ktx2 texture file failed: {0}", ktxErrorString(result));
                return false;
            }
        }

        return true;
    }

    static bool Compress(ktxTexture2* kTexture, const std::filesystem::path& texturePath, bool isHDR, TextureSettings& settings)
    {
        bool compressionResult = false;

        Platform platform = PlatformManager::Instance->GetPlatform();

        switch (platform)
        {
        case Platform::Windows:
        {
            Format format = settings.PlatformCompressionFormat[(int)Platform::Windows];

            if (settings.PlatformCompressionMethod[(int)Platform::Windows] == CompressionMethod::BASISU)
            {
                compressionResult = CompressBasisU(kTexture, texturePath, isHDR, CompressionQualityToUASTC(settings.PlatformCompressionQuality[(int)Platform::Windows]), FormatToTranscodeFormat(format));
            }

            if (compressionResult)
            {
                settings.PixelFormat = settings.PlatformCompressionFormat[(int)Platform::Windows];
            }
            break;
        }
        case Platform::MacOS:
        {
            Format format = settings.PlatformCompressionFormat[(int)Platform::MacOS];

            if (settings.PlatformCompressionMethod[(int)Platform::MacOS] == CompressionMethod::BASISU)
            {
                compressionResult = CompressBasisU(kTexture, texturePath, isHDR, CompressionQualityToUASTC(settings.PlatformCompressionQuality[(int)Platform::MacOS]), FormatToTranscodeFormat(format));
            }
            else if (settings.PlatformCompressionMethod[(int)Platform::MacOS] == CompressionMethod::ASTC)
            {
                compressionResult = CompressAstc(kTexture, texturePath, isHDR, CompressionQualityToASTC(settings.PlatformCompressionQuality[(int)Platform::MacOS]), FormatToASTCBlockFormat(format));
            }

            if (compressionResult)
            {
                settings.PixelFormat = format;
            }
            break;
        }
        case Platform::Linux:
        {
            Format format = settings.PlatformCompressionFormat[(int)Platform::Linux];

            if (settings.PlatformCompressionMethod[(int)Platform::Linux] == CompressionMethod::BASISU)
            {
                compressionResult = CompressBasisU(kTexture, texturePath, isHDR, CompressionQualityToUASTC(settings.PlatformCompressionQuality[(int)Platform::Linux]), FormatToTranscodeFormat(format));
            }

            if (compressionResult)
            {
                settings.PixelFormat = settings.PlatformCompressionFormat[(int)Platform::Linux];
            }
            break;
        }
        case Platform::Web:
        {
            break;
        }
        }

        return compressionResult;
    }

	TextureUtilities& TextureUtilities::Get()
	{
		static TextureUtilities instance;
		return instance;
	}

	void* TextureUtilities::Load(const std::string& path, TextureSettings& settings)
	{
		HBL2_FUNC_PROFILE();

        Platform platform = PlatformManager::Instance->GetPlatform();

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

            std::vector<uint16_t> hdrHalfPixels;
			const auto& pathAsPath = std::filesystem::path(path);
            ktx_uint32_t ktxFormat = 0;
            ktx_size_t pixelDataSize = 0;

			if (pathAsPath.extension() == ".hdr")
			{
                float* hdrPixels = stbi_loadf(path.c_str(), &settings.Width, &settings.Height, &bits, STBI_rgb_alpha);
                
                if (settings.PlatformCompressionMethod[(int)platform] == CompressionMethod::NONE)
                {
                    pixels = hdrPixels;
                    settings.PixelFormat = Format::RGBA32_FLOAT;
                    pixelDataSize = (ktx_size_t)settings.Width * (ktx_size_t)settings.Height * 4 * sizeof(float);
                }
                else
                {
                    // Convert to 16-bit floats for compression to work.
                    const size_t texelCount = static_cast<size_t>(settings.Width) * static_cast<size_t>(settings.Height);
                    const size_t componentCount = texelCount * 4;

                    hdrHalfPixels.resize(componentCount);

                    for (size_t i = 0; i < componentCount; ++i)
                    {
                        hdrHalfPixels[i] = FloatToHalf(hdrPixels[i]);
                    }

                    stbi_image_free(hdrPixels);

                    settings.PixelFormat = Format::RGBA16_FLOAT;
                    pixelDataSize = componentCount * sizeof(uint16_t);

                    ktxFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

                    pixels = hdrHalfPixels.data();
                }
			}
			else
			{
				pixels = stbi_load(path.c_str(), &settings.Width, &settings.Height, &bits, STBI_rgb_alpha);

                if (settings.PlatformCompressionMethod[(int)platform] == CompressionMethod::NONE)
                {
                    settings.PixelFormat = Format::RGBA8_RGB;
                }
                else
                {
                    pixelDataSize = (ktx_size_t)settings.Width * (ktx_size_t)settings.Height * 4;
                    ktxFormat = VK_FORMAT_R8G8B8A8_SRGB;
                }                
			}
            
            HBL2_CORE_ASSERT(pixels, "Failed to load pixels!");
            
            if (settings.PlatformCompressionMethod[(int)platform] == CompressionMethod::NONE)
            {
                return pixels;
            }
            
            ktxTexture2* kTexture;

            const auto& cachedPath = std::filesystem::path("assets") / "cache" / "texture" / (pathAsPath.stem().string() + ".ktx2");
            const auto& workingDir = Project::GetProjectDirectory().parent_path();
            auto texturePath = workingDir / cachedPath;
            
            // Ensure parent path exists.
            if (!std::filesystem::exists(texturePath.parent_path()))
            {
                std::error_code ec;
                std::filesystem::create_directories(texturePath.parent_path(), ec);
                
                if (!ec)
                {
                    HBL2_CORE_ERROR("Texture cache directory creation failed: {0}", ec.message());
                    return nullptr;
                }
            }

            // Check for cache hit for texture.
            if (std::filesystem::exists(texturePath) && !settings.Relaod)
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
                    bool compressionResult = Compress(kTexture, texturePath, ktxFormat == VK_FORMAT_R16G16B16A16_SFLOAT, settings);

                    if (!compressionResult)
                    {
                        return nullptr;
                    }
                }
                else
                {
                    settings.PixelFormat = settings.PlatformCompressionFormat[(int)platform];
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
                    pixelDataSize);

                if (result != KTX_SUCCESS)
                {
                    ktxTexture_Destroy(ktxTexture(kTexture));
                    
                    HBL2_CORE_ERROR("Failed to set ktx2 texture image data: {0}", ktxErrorString(result));
                    return nullptr;
                }

                // Compress ktx2 texture data.
                bool compressionResult = Compress(kTexture, texturePath, ktxFormat == VK_FORMAT_R16G16B16A16_SFLOAT, settings);

                if (!compressionResult)
                {
                    return nullptr;
                }
            }
            
            // Get and return raw pixel data.
            ktx_size_t imageOffset = 0;
            ktxTexture_GetImageOffset(ktxTexture(kTexture), 0, 0, 0, &imageOffset);
            ktx_size_t levelSize = ktxTexture_GetImageSize(ktxTexture(kTexture), 0);

            void* textureData = std::malloc(levelSize);

            if (textureData == nullptr)
            {
                return nullptr;
            }

            std::memcpy(textureData, ktxTexture_GetData(ktxTexture(kTexture)) + imageOffset, levelSize);
            pixelDataSize = levelSize;

            ktxTexture_Destroy(ktxTexture(kTexture));
            
            if (hdrHalfPixels.size() == 0)
            {
                stbi_image_free(pixels);
            }
            
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
        
        out << YAML::Key << "Compression" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "Windows" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "Method" << YAML::Value << (uint32_t)CompressionMethod::NONE;
        out << YAML::Key << "Format" << YAML::Value << (uint32_t)Format::BC7_SRGB;
        out << YAML::Key << "Quality" << YAML::Value << (uint32_t)CompressionQuality::MEDIUM;
        out << YAML::EndMap;
        out << YAML::Key << "MacOS" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "Method" << YAML::Value << (uint32_t)CompressionMethod::NONE;
        out << YAML::Key << "Format" << YAML::Value << (uint32_t)Format::ASTC_4x4_SRGB;
        out << YAML::Key << "Quality" << YAML::Value << (uint32_t)CompressionQuality::MEDIUM;
        out << YAML::EndMap;
        out << YAML::Key << "Linux" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "Method" << YAML::Value << (uint32_t)CompressionMethod::NONE;
        out << YAML::Key << "Format" << YAML::Value << (uint32_t)Format::BC7_SRGB;
        out << YAML::Key << "Quality" << YAML::Value << (uint32_t)CompressionQuality::MEDIUM;
        out << YAML::EndMap;
        out << YAML::Key << "Web" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "Method" << YAML::Value << (uint32_t)CompressionMethod::NONE;
        out << YAML::Key << "Format" << YAML::Value << (uint32_t)Format::BC7_SRGB;
        out << YAML::Key << "Quality" << YAML::Value << (uint32_t)CompressionQuality::MEDIUM;
        out << YAML::EndMap;
        out << YAML::EndMap;
        
		out << YAML::EndMap;
		out << YAML::EndMap;
		fout << out.c_str();
		fout.close();
	}

	void TextureUtilities::SerializeAssetMetadataFile(Handle<Asset> handle, const TextureSettings& settings)
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
        
        out << YAML::Key << "Compression" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "Windows" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "Method" << YAML::Value << (uint32_t)settings.PlatformCompressionMethod[(int)Platform::Windows];
        out << YAML::Key << "Format" << YAML::Value << (uint32_t)settings.PlatformCompressionFormat[(int)Platform::Windows];
        out << YAML::Key << "Quality" << YAML::Value << (uint32_t)settings.PlatformCompressionQuality[(int)Platform::Windows];
        out << YAML::EndMap;
        out << YAML::Key << "MacOS" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "Method" << YAML::Value << (uint32_t)settings.PlatformCompressionMethod[(int)Platform::MacOS];
        out << YAML::Key << "Format" << YAML::Value << (uint32_t)settings.PlatformCompressionFormat[(int)Platform::MacOS];
        out << YAML::Key << "Quality" << YAML::Value << (uint32_t)settings.PlatformCompressionQuality[(int)Platform::MacOS];
        out << YAML::EndMap;
        out << YAML::Key << "Linux" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "Method" << YAML::Value << (uint32_t)settings.PlatformCompressionMethod[(int)Platform::Linux];
        out << YAML::Key << "Format" << YAML::Value << (uint32_t)settings.PlatformCompressionFormat[(int)Platform::Linux];
        out << YAML::Key << "Quality" << YAML::Value << (uint32_t)settings.PlatformCompressionQuality[(int)Platform::Linux];
        out << YAML::EndMap;
        out << YAML::Key << "Web" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "Method" << YAML::Value << (uint32_t)settings.PlatformCompressionMethod[(int)Platform::Web];
        out << YAML::Key << "Format" << YAML::Value << (uint32_t)settings.PlatformCompressionFormat[(int)Platform::Web];
        out << YAML::Key << "Quality" << YAML::Value << (uint32_t)settings.PlatformCompressionQuality[(int)Platform::Web];
        out << YAML::EndMap;
        out << YAML::EndMap;
        
		out << YAML::EndMap;
		out << YAML::EndMap;
		fout << out.c_str();
		fout.close();
	}

    TextureSettings TextureUtilities::DeserializeAssetMetadataFile(Handle<Asset> handle)
    {
        if (!AssetManager::Instance->IsAssetValid(handle))
        {
            return {};
        }

        Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);

        return DeserializeAssetMetadataFile(asset);
    }

    TextureSettings TextureUtilities::DeserializeAssetMetadataFile(Asset* asset)
    {
        if (asset == nullptr)
        {
            return {};
        }

        const std::filesystem::path& filePath = HBL2::Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hbltexture";

        TextureSettings settings = {};

        if (!std::filesystem::exists(filePath))
        {
            return settings;
        }

        try
        {
            YAML::Node data = YAML::LoadFile(filePath.string());

            YAML::Node textureNode = data["Texture"];

            if (!textureNode)
            {
                return settings;
            }

            if (textureNode["Flip"])
            {
                settings.Flip = textureNode["Flip"].as<bool>();
            }

            YAML::Node compressionNode = textureNode["Compression"];

            if (!compressionNode)
            {
                return settings;
            }

            static const std::pair<const char*, Platform> platformKeys[] =
            {
                { "Windows", Platform::Windows },
                { "MacOS",   Platform::MacOS   },
                { "Linux",   Platform::Linux   },
                { "Web",     Platform::Web     },
            };

            for (const auto& [key, platform] : platformKeys)
            {
                YAML::Node platformNode = compressionNode[key];

                if (!platformNode)
                {
                    continue;
                }

                const int index = (int)platform;

                if (platformNode["Method"].IsDefined())
                {
                    settings.PlatformCompressionMethod[index] = (CompressionMethod)platformNode["Method"].as<uint32_t>();
                }

                if (platformNode["Format"].IsDefined())
                {
                    settings.PlatformCompressionFormat[index] = (Format)platformNode["Format"].as<uint32_t>();
                }

                if (platformNode["Quality"].IsDefined())
                {
                    settings.PlatformCompressionQuality[index] = (CompressionQuality)platformNode["Quality"].as<uint32_t>();
                }
            }
        }
        catch (const YAML::Exception& e)
        {
            HBL2_CORE_ERROR("Failed to deserialize texture metadata '{0}': {1}", filePath.string(), e.what());
            return {};
        }

        return settings;
    }

	void TextureUtilities::LoadWhiteTexture()
	{
		WhiteTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "white-texture",
			.dimensions = { 1.0f, 1.0f, 1.0f },
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
