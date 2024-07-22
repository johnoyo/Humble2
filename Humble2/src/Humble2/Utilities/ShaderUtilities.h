#pragma once

#include "Base.h"
#include "Renderer\Rewrite\Renderer.h"
#include "Renderer\Rewrite\Enums.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include <fstream>
#include <filesystem>

namespace HBL2
{
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

		std::string ReadFile(const std::string& filepath)
		{
			std::string result;
			std::ifstream in(filepath, std::ios::in | std::ios::binary);
			if (in)
			{
				in.seekg(0, std::ios::end);
				size_t size = in.tellg();
				if (size != -1)
				{
					result.resize(size);
					in.seekg(0, std::ios::beg);
					in.read(&result[0], size);
				}
				else
				{
					HBL2_CORE_ERROR("Could not read from file '{0}'", filepath);
				}
			}
			else
			{
				HBL2_CORE_ERROR("Could not open file '{0}'", filepath);
			}

			return result;
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

		std::vector<uint32_t> Compile(const std::string& shaderFilePath, ShaderStage stage)
		{
			GraphicsAPI target = Renderer::Instance->GetAPI();

			CreateCacheDirectoryIfNeeded(target);

			std::vector<uint32_t> vulkanShaderData;
			std::vector<uint32_t> openGLShaderData;
			std::filesystem::path shaderPath = shaderFilePath;
			std::filesystem::path cacheDirectory = GetCacheDirectory(target);

			{
				shaderc::Compiler compiler;
				shaderc::CompileOptions options;
				options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);

				switch (target)
				{
				case GraphicsAPI::OPENGL:
					options.AddMacroDefinition("OpenGL");
					break;
				case GraphicsAPI::VULKAN:
					options.AddMacroDefinition("Vulkan");
					break;
				}

				options.SetOptimizationLevel(shaderc_optimization_level_performance);

				std::filesystem::path cachedPath = cacheDirectory / (shaderPath.filename().string() + GLShaderStageCachedVulkanFileExtension(stage));

				std::string source = ReadFile(shaderFilePath);

				std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
				if (in.is_open())
				{
					in.seekg(0, std::ios::end);
					auto size = in.tellg();
					in.seekg(0, std::ios::beg);

					vulkanShaderData.resize(size / sizeof(uint32_t));
					in.read((char*)vulkanShaderData.data(), size);
				}
				else
				{
					shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, GLShaderStageToShaderC(stage), shaderFilePath.c_str(), options);
					if (module.GetCompilationStatus() != shaderc_compilation_status_success)
					{
						HBL2_CORE_ERROR(module.GetErrorMessage());
						assert(false);
						// HBL2_CORE_ASSERT(false);
					}

					vulkanShaderData = std::vector<uint32_t>(module.cbegin(), module.cend());

					std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
					if (out.is_open())
					{
						out.write((char*)vulkanShaderData.data(), vulkanShaderData.size() * sizeof(uint32_t));
						out.flush();
						out.close();
					}
				}
			}

			// TODO: Reflect here
			Reflect(stage, vulkanShaderData);

			switch (target)
			{
			case GraphicsAPI::VULKAN:
				return vulkanShaderData;
			case GraphicsAPI::OPENGL:
				{
					shaderc::Compiler compiler;
					shaderc::CompileOptions options;
					options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
					options.SetOptimizationLevel(shaderc_optimization_level_performance);

					std::filesystem::path cachedPath = cacheDirectory / (shaderPath.filename().string() + GLShaderStageCachedOpenGLFileExtension(stage));

					std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
					if (in.is_open())
					{
						in.seekg(0, std::ios::end);
						auto size = in.tellg();
						in.seekg(0, std::ios::beg);

						openGLShaderData.resize(size / sizeof(uint32_t));
						in.read((char*)openGLShaderData.data(), size);
					}
					else
					{
						spirv_cross::CompilerGLSL glslCompiler(vulkanShaderData);
						glslCompiler.set_common_options({
							.version = 310,
							.es = true,
						});
						const auto& source = glslCompiler.compile();

						shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, GLShaderStageToShaderC(stage), shaderFilePath.c_str(), options);
						if (module.GetCompilationStatus() != shaderc_compilation_status_success)
						{
							HBL2_CORE_ERROR(module.GetErrorMessage());
							// HBL2_CORE_ASSERT(false);
							assert(false);
						}

						openGLShaderData = std::vector<uint32_t>(module.cbegin(), module.cend());

						std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
						if (out.is_open())
						{
							out.write((char*)openGLShaderData.data(), openGLShaderData.size() * sizeof(uint32_t));
							out.flush();
							out.close();
						}
					}
				}
				return openGLShaderData;
			default:
				HBL2_CORE_ERROR("Target API not supported");
				assert(false);
				return std::vector<uint32_t>();
			}			
		}

		void Reflect(ShaderStage stage, const std::vector<uint32_t>& shaderData)
		{
			spirv_cross::Compiler compiler(shaderData);
			spirv_cross::ShaderResources resources = compiler.get_shader_resources();

			HBL2_CORE_TRACE("OpenGLShader::Reflect - {0}", GLShaderStageToString(stage));
			HBL2_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
			HBL2_CORE_TRACE("    {0} resources", resources.sampled_images.size());

			HBL2_CORE_TRACE("Uniform buffers:");
			for (const auto& resource : resources.uniform_buffers)
			{
				const auto& bufferType = compiler.get_type(resource.base_type_id);
				uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
				uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				int memberCount = bufferType.member_types.size();

				HBL2_CORE_TRACE("  {0}", resource.name);
				HBL2_CORE_TRACE("    Size = {0}", bufferSize);
				HBL2_CORE_TRACE("    Binding = {0}", binding);
				HBL2_CORE_TRACE("    Members = {0}", memberCount);
			}
		}

	private:
		ShaderUtilities() = default;
	};
}