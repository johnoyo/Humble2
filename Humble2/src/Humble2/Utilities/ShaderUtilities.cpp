#include "ShaderUtilities.h"
#include <iostream>

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

		GraphicsAPI api = Renderer::Instance->GetAPI();
		std::filesystem::path shaderPath = shaderFilePath;
		std::filesystem::path cacheDirectory = GetCacheDirectory(api);
		const char* vertexCachedVulkanFileExtension = GLShaderStageCachedVulkanFileExtension(ShaderStage::VERTEX);
		const char* fragmentCachedVulkanFileExtension = GLShaderStageCachedVulkanFileExtension(ShaderStage::FRAGMENT);
		std::filesystem::path vertexShaderPath = cacheDirectory / (shaderPath.filename().string() + vertexCachedVulkanFileExtension);
		std::filesystem::path fragmentShaderPath = cacheDirectory / (shaderPath.filename().string() + fragmentCachedVulkanFileExtension);

		std::vector<uint32_t> vertexShaderData;
		std::ifstream inV(vertexShaderPath, std::ios::in | std::ios::binary);
		if (inV.is_open())
		{
			inV.seekg(0, std::ios::end);
			auto size = inV.tellg();
			inV.seekg(0, std::ios::beg);

			vertexShaderData.resize(size / sizeof(uint32_t));
			inV.read((char*)vertexShaderData.data(), size);
		}

		std::vector<uint32_t> fragmentShaderData;
		std::ifstream inF(fragmentShaderPath, std::ios::in | std::ios::binary);
		if (inF.is_open())
		{
			inF.seekg(0, std::ios::end);
			auto size = inF.tellg();
			inF.seekg(0, std::ios::beg);

			fragmentShaderData.resize(size / sizeof(uint32_t));
			inF.read((char*)fragmentShaderData.data(), size);
		}

		HBL2_CORE_TRACE("Reflecting Shader: {0}", shaderFilePath);

		m_ShaderReflectionData[shaderFilePath] = Reflect(vertexShaderData, fragmentShaderData);

		return shaderBinaries;
	}

	void ShaderUtilities::AddShader(BuiltInShader shader, const ShaderDescriptor&& desc)
	{
		Handle<Shader> shaderHandle = ResourceManager::Instance->CreateShader(std::forward<const ShaderDescriptor>(desc));
		m_Shaders[shader] = shaderHandle;
	}

	ReflectionData ShaderUtilities::Reflect(const std::vector<uint32_t>& vertexShaderData, const std::vector<uint32_t>& fragmentShaderData)
	{
		ReflectionData reflectionData;

		// Vertex
		{
			spirv_cross::Compiler compiler(vertexShaderData);
			spirv_cross::ShaderResources resources = compiler.get_shader_resources();

			HBL2_CORE_TRACE("OpenGLShader::Reflect - {0}", GLShaderStageToString(ShaderStage::VERTEX));
			HBL2_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
			HBL2_CORE_TRACE("    {0} storage buffers", resources.storage_buffers.size());
			HBL2_CORE_TRACE("    {0} resources", resources.sampled_images.size());

			// Get all entry points and their execution stages
			auto entryPoints = compiler.get_entry_points_and_stages();

			std::cout << "Shader Entry Points:" << std::endl;
			for (const auto& entryPoint : entryPoints)
			{
				std::cout << "  Name: " << entryPoint.name << std::endl;

				// Print the shader stage
				std::cout << "  Stage: ";
				switch (entryPoint.execution_model)
				{
				case spv::ExecutionModelVertex:
					std::cout << "Vertex Shader" << std::endl;
					reflectionData.VertexEntryPoint = entryPoint.name;
					break;
				}
			}

			uint32_t byteStride = 0;

			// Print shader inputs (vertex attributes)
			std::cout << "Shader Inputs (Vertex Attributes):" << std::endl;
			for (const auto& input : resources.stage_inputs)
			{
				uint32_t location = compiler.get_decoration(input.id, spv::DecorationLocation);
				const spirv_cross::SPIRType& type = compiler.get_type(input.type_id);

				// Print basic information
				std::cout << "  Name: " << input.name << "\n";
				std::cout << "  Location: " << location << "\n";

				// Print type information (e.g., vec3, float, vec4)
				std::cout << "  Type: ";
				if (type.basetype == spirv_cross::SPIRType::Float && type.vecsize == 1 && type.columns == 1)
				{
					reflectionData.Attributes.push_back({ byteStride, VertexFormat::FLOAT32 });
					byteStride += sizeof(float);
					std::cout << "float";
				}
				else if (type.basetype == spirv_cross::SPIRType::Float && type.vecsize > 1)
				{
					VertexFormat vertexFormat;

					switch (type.vecsize)
					{
					case 2:
						vertexFormat = VertexFormat::FLOAT32x2;
						break;
					case 3:
						vertexFormat = VertexFormat::FLOAT32x3;
						break;
					case 4:
						vertexFormat = VertexFormat::FLOAT32x4;
						break;
					default:
						break;
					}

					reflectionData.Attributes.push_back({ byteStride, vertexFormat });
					byteStride += sizeof(float) * type.vecsize;
					std::cout << "vec" << type.vecsize;
				}
				else if (type.basetype == spirv_cross::SPIRType::Int && type.vecsize == 1)
				{
					reflectionData.Attributes.push_back({ byteStride, VertexFormat::INT32 });
					byteStride += sizeof(int);
					std::cout << "int";
				}
				else if (type.basetype == spirv_cross::SPIRType::Int && type.vecsize > 1)
				{
					VertexFormat vertexFormat;

					switch (type.vecsize)
					{
					case 2:
						vertexFormat = VertexFormat::INT32x2;
						break;
					case 3:
						vertexFormat = VertexFormat::INT32x3;
						break;
					case 4:
						vertexFormat = VertexFormat::INT32x4;
						break;
					default:
						break;
					}

					reflectionData.Attributes.push_back({ byteStride, vertexFormat });
					byteStride += sizeof(int) * type.vecsize;
					std::cout << "ivec" << type.vecsize;
				}
				else if (type.basetype == spirv_cross::SPIRType::UInt && type.vecsize == 1)
				{
					reflectionData.Attributes.push_back({ byteStride, VertexFormat::UINT32 });
					byteStride += sizeof(int);
					std::cout << "int";
				}
				else if (type.basetype == spirv_cross::SPIRType::UInt && type.vecsize > 1)
				{
					VertexFormat vertexFormat;

					switch (type.vecsize)
					{
					case 2:
						vertexFormat = VertexFormat::UINT32x2;
						break;
					case 3:
						vertexFormat = VertexFormat::UINT32x3;
						break;
					case 4:
						vertexFormat = VertexFormat::UINT32x4;
						break;
					default:
						break;
					}

					reflectionData.Attributes.push_back({ byteStride, vertexFormat });
					byteStride += sizeof(unsigned int) * type.vecsize;
					std::cout << "uvec" << type.vecsize;
				}
				else if (type.basetype == spirv_cross::SPIRType::Float && type.columns > 1)
				{
					byteStride += sizeof(float) * type.columns * type.vecsize;
					std::cout << "mat" << type.columns << "x" << type.vecsize;
				}
				else
				{
					std::cout << "unknown";
				}
				std::cout << "\n";

				reflectionData.ByteStride = byteStride;

				// Check if it is an array and print the array size
				if (type.array.size() > 0)
				{
					std::cout << "  Array Size: ";
					if (type.array_size_literal[0])
					{
						std::cout << type.array[0] << "\n";
					}
					else
					{
						std::cout << "Runtime Array\n";
					}
				}

				// Print component index if applicable
				if (compiler.has_decoration(input.id, spv::DecorationComponent))
				{
					uint32_t componentIndex = compiler.get_decoration(input.id, spv::DecorationComponent);
					std::cout << "  Component Index: " << componentIndex << "\n";
				}

				std::cout << std::endl;
			}

			// Print uniform buffers
			std::cout << "Uniform Buffers:" << std::endl;
			for (const auto& ubo : resources.uniform_buffers)
			{
				const auto& bufferType = compiler.get_type(ubo.base_type_id);
				uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
				uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
				std::cout << "  Name: " << ubo.name << ", Set: " << set << ", Binding: " << binding << ", Size: " << bufferSize << std::endl;
			}

			// Print storage buffers
			std::cout << "Storage Buffers:" << std::endl;
			for (const auto& ssbo : resources.storage_buffers) 
			{
				const auto& bufferType = compiler.get_type(ssbo.base_type_id);
				uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
				uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
				std::cout << "  Name: " << ssbo.name << ", Set: " << set << ", Binding: " << binding << ", Size: " << bufferSize << std::endl;
			}

			// Print sampled images (textures)
			std::cout << "Sampled Images (Textures):" << std::endl;
			for (const auto& sampler : resources.sampled_images)
			{
				uint32_t binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
				std::cout << "  Name: " << sampler.name << ", Set: " << set << ", Binding: " << binding << std::endl;
			}

			// Print push constants
			std::cout << "Push Constants:" << std::endl;
			for (const auto& push_constant : resources.push_constant_buffers)
			{
				const spirv_cross::SPIRType& type = compiler.get_type(push_constant.base_type_id);
				std::cout << "  Name: " << push_constant.name << ", Size: " << compiler.get_declared_struct_size(type) << " bytes" << std::endl;
			}
		}

		// Fragment
		{
			spirv_cross::Compiler compiler(fragmentShaderData);
			spirv_cross::ShaderResources resources = compiler.get_shader_resources();

			HBL2_CORE_TRACE("OpenGLShader::Reflect - {0}", GLShaderStageToString(ShaderStage::FRAGMENT));
			HBL2_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
			HBL2_CORE_TRACE("    {0} resources", resources.sampled_images.size());

			// Get all entry points and their execution stages
			auto entryPoints = compiler.get_entry_points_and_stages();

			std::cout << "Shader Entry Points:" << std::endl;
			for (const auto& entryPoint : entryPoints)
			{
				std::cout << "  Name: " << entryPoint.name << std::endl;

				// Print the shader stage
				std::cout << "  Stage: ";
				switch (entryPoint.execution_model)
				{
				case spv::ExecutionModelFragment:
					std::cout << "Fragment Shader" << std::endl;
					reflectionData.FragmentEntryPoint = entryPoint.name;
					break;
				}
			}

			// Print uniform buffers
			std::cout << "Uniform Buffers:" << std::endl;
			for (const auto& ubo : resources.uniform_buffers)
			{
				const auto& bufferType = compiler.get_type(ubo.base_type_id);
				uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
				uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
				std::cout << "  Name: " << ubo.name << ", Set: " << set << ", Binding: " << binding << ", Size: " << bufferSize << std::endl;
			}

			// Print sampled images (textures)
			std::cout << "Sampled Images (Textures):" << std::endl;
			for (const auto& sampler : resources.sampled_images)
			{
				uint32_t binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
				std::cout << "  Name: " << sampler.name << ", Set: " << set << ", Binding: " << binding << std::endl;
			}
		}

		return reflectionData;
	}
}
