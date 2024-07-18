#include "Material.h"

namespace HBL
{
	Material::Material(UUID shader)
	{
		Asset* shaderAsset = AssetManager::Get().GetAsset(shader);

		if (shaderAsset != nullptr)
		{
			m_Shader = Shader::Create(shaderAsset->Name, shaderAsset->RelativePath.string());
		}
	}

	void Material::Bind()
	{
	}

	void Material::UnBind()
	{
	}

	void Material::SetValue(const std::string& uniform, float value)
	{
	}

	void Material::SetColor(const std::string& uniform, glm::vec4& color)
	{
	}

	void Material::SetTexture(const std::string& uniform, uint32_t texture)
	{
	}
}