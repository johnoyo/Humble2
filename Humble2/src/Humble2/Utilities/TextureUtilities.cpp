#include "TextureUtilities.h"

#include <Project\Project.h>

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
			pixels = stbi_load(path.c_str(), &settings.Width, &settings.Height, &bits, STBI_rgb_alpha);
			assert(pixels);
		}

		return pixels;
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

	void TextureUtilities::DeleteWhiteTexture()
	{
		ResourceManager::Instance->DeleteTexture(WhiteTexture);
	}
}
