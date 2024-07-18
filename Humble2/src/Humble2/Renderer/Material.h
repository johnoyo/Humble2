#pragma once

#include "Base.h"
#include "Shader.h"
#include "Texture.h"

#include "Utilities\AssetManager.h"

#include <stdint.h>

namespace HBL
{
	enum class CullMode
	{
		TwoSided,
		Front,
		Back,
	};

	enum class DepthFunc
	{
		Less,
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always,
	};

	class Material
	{
	public:
		Material(UUID shader);

		void Bind();
		void UnBind();

		void SetValue(const std::string& uniform, float value);
		void SetColor(const std::string& uniform, glm::vec4& color);
		void SetTexture(const std::string& uniform, uint32_t texture);

	private:
		CullMode m_CullMode = CullMode::Back;

		bool m_DepthEnabled = true;
		DepthFunc m_DepthFunc = DepthFunc::Less;

		bool m_BlendEnabled = true;

		float m_Shininess = 0.5f;
		float m_Metalicness = 0.5f;
		float m_Roughness = 0.0f;

		glm::vec4 m_Color = { 0.8f, 0.5f, 0.3f, 1.0f };

		Texture* m_AlbedoMap = nullptr;
		Texture* m_NormalMap = nullptr;
		Texture* m_MetalicMap = nullptr;

		Shader* m_Shader = nullptr;
	};
}