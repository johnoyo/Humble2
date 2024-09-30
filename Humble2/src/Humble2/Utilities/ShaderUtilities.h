#pragma once

#include "Base.h"
#include "Renderer\Rewrite\Renderer.h"
#include "Renderer\Rewrite\Enums.h"

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
	enum class BuiltInShader
	{
		INVALID = 0,
		UNLIT,
		BLINN_PHONG,
		PBR,
		TEXTURED_SPRITE,
	};

	struct ReflectionData
	{
		std::string VertexEntryPoint;
		std::string FragmentEntryPoint;
		uint32_t VertexBindingCount = 0;
		uint32_t ByteStride = 0;
		std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding::Attribute> Attributes;
	};

	class ShaderUtilities
	{
	public:
		ShaderUtilities(const ShaderUtilities&) = delete;

		static ShaderUtilities& Get()
		{
			static ShaderUtilities instance;
			return instance;
		}

		const char* GetCacheDirectory(GraphicsAPI target)
		{
			switch (target)
			{
			case GraphicsAPI::OPENGL:
				return "assets/cache/shader/opengl";
			case GraphicsAPI::VULKAN:
				return "assets/cache/shader/vulkan";
			default:
				HBL2_CORE_ERROR("Stage not supported");
				assert(false);
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
				HBL2_CORE_ERROR("Stage not supported");
				assert(false);
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
				HBL2_CORE_ERROR("Stage not supported");
				assert(false);
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
			}
			// HBL2_CORE_ASSERT(false);
			assert(false);
			return (shaderc_shader_kind)0;
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
				// HBL2_CORE_ASSERT(false);
				assert(false);
				return "";
			}
		}

		std::string ReadFile(const std::string& filepath);

		std::vector<std::vector<uint32_t>> Compile(const std::string& shaderFilePath);

		const ReflectionData& GetReflectionData(const std::string& shaderFilePath) { return m_ShaderReflectionData[shaderFilePath]; }

		void LoadBuiltInShaders();

		Handle<Shader> GetBuiltInShader(BuiltInShader shader) { return m_Shaders[shader]; }

	private:
		ShaderUtilities() = default;
		std::vector<uint32_t> Compile(const std::string& shaderFilePath, const std::string& shaderSource, ShaderStage stage);
		ReflectionData Reflect(const std::vector<uint32_t>& vertexShaderData, const std::vector<uint32_t>& fragmentShaderData);

	private:
		std::unordered_map<std::string, ReflectionData> m_ShaderReflectionData;
		std::unordered_map<BuiltInShader, Handle<Shader>> m_Shaders;
	};
}