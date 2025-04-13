#pragma once

#include "Base.h"
#include "Renderer\Renderer.h"
#include "Renderer\Enums.h"

#include "Asset\AssetManager.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include <fstream>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <iostream>

namespace HBL2
{
	enum class HBL2_API BuiltInShader
	{
		INVALID = 0,
		PRESENT,
		UNLIT,
		BLINN_PHONG,
		PBR,
	};

	struct HBL2_API ReflectionData
	{
		std::string VertexEntryPoint;
		std::string FragmentEntryPoint;
		uint32_t VertexBindingCount = 0;
		uint32_t ByteStride = 0;
		std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding::Attribute> Attributes;
	};

	struct HBL2_API MaterialDataDescriptor
	{
		Handle<Asset> ShaderAssetHandle;

		ShaderDescriptor::RenderPipeline::Variant VariantDescriptor{};
		glm::vec4 AlbedoColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		float Glossiness = 0.0f;

		Handle<Asset> AlbedoMapAssetHandle;
		Handle<Asset> NormalMapAssetHandle;
		Handle<Asset> RoughnessMapAssetHandle;
		Handle<Asset> MetallicMapAssetHandle;
	};

	class HBL2_API ShaderUtilities
	{
	public:
		ShaderUtilities(const ShaderUtilities&) = delete;

		static ShaderUtilities& Get();

		const char* GetCacheDirectory(GraphicsAPI target)
		{
			switch (target)
			{
			case GraphicsAPI::OPENGL:
				return "assets/cache/shader/opengl";
			case GraphicsAPI::VULKAN:
				return "assets/cache/shader/vulkan";
			default:
				HBL2_CORE_ASSERT(false, "Stage not supported");
				return "";
			}
		}

		void CreateCacheDirectoryIfNeeded(GraphicsAPI target)
		{
			std::string cacheDirectory = GetCacheDirectory(target);

			if (!std::filesystem::exists(cacheDirectory))
			{
				std::filesystem::create_directories(cacheDirectory);
			}
		}

		const char* GLShaderStageCachedVulkanFileExtension(ShaderStage stage)
		{
			switch (stage)
			{
			case ShaderStage::VERTEX:
				return ".cached_vulkan.vert";
			case ShaderStage::FRAGMENT:
				return ".cached_vulkan.frag";
			default:
				HBL2_CORE_ASSERT(false, "Stage not supported");
				return "";
			}
		}

		const char* GLShaderStageCachedOpenGLFileExtension(ShaderStage stage)
		{
			switch (stage)
			{
			case ShaderStage::VERTEX:
				return ".cached_opengl.vert";
			case ShaderStage::FRAGMENT:
				return ".cached_opengl.frag";
			default:
				HBL2_CORE_ASSERT(false, "Stage not supported");
				return "";
			}
		}

		shaderc_shader_kind GLShaderStageToShaderC(ShaderStage stage)
		{
			switch (stage)
			{
				case ShaderStage::VERTEX:
					return shaderc_glsl_vertex_shader;
				case ShaderStage::FRAGMENT:
					return shaderc_glsl_fragment_shader;
				default:
					HBL2_CORE_ASSERT(false, "Stage not supported");
					return (shaderc_shader_kind)0;
			}
		}

		const char* GLShaderStageToString(ShaderStage stage)
		{
			switch (stage)
			{
			case ShaderStage::VERTEX:
				return "ShaderStage::VERTEX";
			case ShaderStage::FRAGMENT:
				return "ShaderStage::FRAGMENT";
			default:
				HBL2_CORE_ASSERT(false, "Stage not supported");
				return "";
			}
		}

		std::string ReadFile(const std::string& filepath);

		std::vector<std::vector<uint32_t>> Compile(const std::string& shaderFilePath);

		const ReflectionData& GetReflectionData(const std::string& shaderFilePath) { return m_ShaderReflectionData[shaderFilePath]; }

		void LoadBuiltInShaders();
		void DeleteBuiltInShaders();

		Handle<Shader> GetBuiltInShader(BuiltInShader shader) { return m_Shaders[shader]; }
		Handle<BindGroupLayout> GetBuiltInShaderLayout(BuiltInShader shader) { return m_ShaderLayouts[shader]; }

		void CreateShaderMetadataFile(Handle<Asset> handle, uint32_t shaderType);
		void UpdateShaderVariantMetadataFile(UUID shaderUUID, const ShaderDescriptor::RenderPipeline::Variant& newVariant);

		void CreateMaterialMetadataFile(Handle<Asset> handle, uint32_t materialType);
		void CreateMaterialAssetFile(Handle<Asset> handle, const MaterialDataDescriptor&& desc);

	private:
		ShaderUtilities() = default;
		std::vector<uint32_t> Compile(const std::string& shaderFilePath, const std::string& shaderSource, ShaderStage stage);
		ReflectionData Reflect(const Span<uint32_t>& vertexShaderData, const Span<uint32_t>& fragmentShaderData);

	private:
		std::unordered_map<std::string, ReflectionData> m_ShaderReflectionData;
		std::unordered_map<BuiltInShader, Handle<Shader>> m_Shaders;
		std::unordered_map<BuiltInShader, Handle<BindGroupLayout>> m_ShaderLayouts;
	};
}