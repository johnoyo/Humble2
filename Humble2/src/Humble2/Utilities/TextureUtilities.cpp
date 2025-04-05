#include "TextureUtilities.h"

#include <Project\Project.h>

#include <stb_image\stb_image_write.h>

namespace HBL2
{
	TextureUtilities& TextureUtilities::Get()
	{
		static TextureUtilities instance;
		return instance;
	}

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
			else
			{
				stbi_set_flip_vertically_on_load(0);
			}
			pixels = stbi_load(path.c_str(), &settings.Width, &settings.Height, &bits, STBI_rgb_alpha);
			assert(pixels);
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

		stbi_uc* pixels = stbi_load_from_memory((const stbi_uc*)bytes.Data(), bytes.Size(), &width, &height, &channels, STBI_default);

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

		std::ofstream fout(filePath, 0);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Texture" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(handle)->UUID;
		out << YAML::Key << "Flip" << YAML::Value << false;
		out << YAML::EndMap;
		out << YAML::EndMap;
		fout << out.c_str();
		fout.close();
	}

	void TextureUtilities::UpdateAssetMetadataFile(Handle<Asset> handle, bool flip)
	{
		if (!AssetManager::Instance->IsAssetValid(handle))
		{
			return;
		}

		Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
		const std::filesystem::path& filePath = HBL2::Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hbltexture";

		std::ofstream fout(filePath, 0);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Texture" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(handle)->UUID;
		out << YAML::Key << "Flip" << YAML::Value << flip;
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
