#include "ShaderUtilities.h"

namespace HBL2
{
	std::string ShaderUtilities::ReadFile(const std::string& filepath)
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

	std::vector<uint32_t> ShaderUtilities::Compile(const std::string& shaderFilePath, const std::string& shaderSource, ShaderStage stage)
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
				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(shaderSource, GLShaderStageToShaderC(stage), shaderFilePath.c_str(), options);
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

		HBL2_CORE_TRACE("Reflecting Shader: {0}", shaderFilePath);

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

	std::vector<std::vector<uint32_t>> ShaderUtilities::Compile(const std::string& shaderFilePath)
	{
		std::fstream newFile;

		enum class ShaderType
		{
			NONE = -1, VERTEX = 0, FRAGMENT = 1
		};

		std::string line;
		std::stringstream ss[2];
		ShaderType type = ShaderType::NONE;

		newFile.open(shaderFilePath, std::ios::in);

		if (newFile.is_open())
		{
			while (getline(newFile, line))
			{
				if (line.find("#shader") != std::string::npos)
				{
					if (line.find("vertex") != std::string::npos)
						type = ShaderType::VERTEX;
					else if (line.find("fragment") != std::string::npos)
						type = ShaderType::FRAGMENT;
				}
				else
				{
					ss[(int)type] << line << '\n';
				}
			}

			// Close the file object.
			newFile.close();
		}
		else
		{
			HBL2_CORE_ERROR("Could not open file: {0}.", shaderFilePath);
		}

		std::vector<std::vector<uint32_t>> shaderBinaries;

		shaderBinaries.push_back(Compile(shaderFilePath, ss[0].str(), ShaderStage::VERTEX));
		shaderBinaries.push_back(Compile(shaderFilePath, ss[1].str(), ShaderStage::FRAGMENT));

		return shaderBinaries;
	}

	void ShaderUtilities::Reflect(ShaderStage stage, const std::vector<uint32_t>& shaderData)
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

		HBL2_CORE_TRACE("Sampled images:");
		for (const auto& resource : resources.sampled_images)
		{
			const auto& samplerType = compiler.get_type(resource.base_type_id);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			HBL2_CORE_TRACE("  {0}", samplerType.basetype);
			HBL2_CORE_TRACE("    Binding = {0}", binding);
		}
	}
}
