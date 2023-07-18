#include "Shader.h"

#include "Renderer2D.h"
#include "Platform/OpenGL/OpenGLShader.h"

namespace HBL2
{
	std::unordered_map<std::string, Shader*> Shader::s_ShaderLib;

	Shader::Shader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
		: m_Name(name), m_VertexSource(vertexSource), m_FragmentSource(fragmentSource)
	{
	}

	Shader* Shader::Create(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource, int tmp)
	{
		if (Exists(name))
			return Get(name);

		switch (RenderCommand::GetAPI())
		{
		case GraphicsAPI::OpenGL:
			return new OpenGLShader(name, vertexSource, fragmentSource);
		case GraphicsAPI::Vulkan:
			HBL2_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLShader(name, vertexSource, fragmentSource);
		case HBL2::GraphicsAPI::None:
			HBL2_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			return nullptr;
		}

		return nullptr;
	}

	Shader* Shader::Create(const std::string& name, const std::string& vertexFilepath, const std::string& fragmentFilepath)
	{
		if (Exists(name))
			return Get(name);

		std::fstream stream;

		std::string line;
		std::stringstream ssV;

		stream.open(vertexFilepath, std::ios::in);

		if (stream.is_open())
		{
			while (getline(stream, line))
			{
				ssV << line << '\n';
			}

			stream.close();
		}
		else
		{
			HBL2_CORE_ERROR("Could not open file: {0}.", vertexFilepath);
		}

		line.clear();
		std::stringstream ssF;

		stream.open(fragmentFilepath, std::ios::in);

		if (stream.is_open())
		{
			while (getline(stream, line))
			{
				ssF << line << '\n';
			}

			stream.close();
		}
		else
		{
			HBL2_CORE_ERROR("Could not open file: {0}.", fragmentFilepath);
		}

		switch (RenderCommand::GetAPI())
		{
		case GraphicsAPI::OpenGL:
			return new OpenGLShader(name, ssV.str(), ssF.str());
		case GraphicsAPI::Vulkan:
			HBL2_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLShader(name, ssV.str(), ssF.str());
		case HBL2::GraphicsAPI::None:
			HBL2_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			return nullptr;
		}

		return nullptr;
	}
	Shader* Shader::Create(const std::string& name, const std::string& filepath)
	{
		if (Exists(name))
			return Get(name);

		std::fstream newFile;

		enum class ShaderType
		{
			NONE = -1, VERTEX = 0, FRAGMENT = 1
		};

		std::string line;
		std::stringstream ss[2];
		ShaderType type = ShaderType::NONE;

		newFile.open(filepath, std::ios::in);

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
			HBL2_CORE_ERROR("Could not open file: {0}.", filepath);
		}

		switch (RenderCommand::GetAPI())
		{
		case GraphicsAPI::OpenGL:
			return new OpenGLShader(name, ss[0].str(), ss[1].str());
		case GraphicsAPI::Vulkan:
			HBL2_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLShader(name, ss[0].str(), ss[1].str());
		case HBL2::GraphicsAPI::None:
			HBL2_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			return nullptr;
		}

		return nullptr;
	}

	void Shader::Add(const std::string& name, Shader* shader)
	{
		assert(!Exists(name));
		s_ShaderLib[name] = shader;
	}

	Shader* Shader::Get(const std::string& name)
	{
		assert(Exists(name));
		return s_ShaderLib[name];
	}

	bool Shader::Exists(const std::string& name)
	{
		return s_ShaderLib.find(name) != s_ShaderLib.end();
	}

	void Shader::Clean()
	{
		for (auto& item : s_ShaderLib)
		{
			delete item.second;
		}
	}
}