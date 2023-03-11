#include "Shader.h"

#include "Renderer2D.h"
#include "../../Platform/OpenGL/OpenGLShader.h"

namespace HBL
{
	std::unordered_map<std::string, Shader*> Shader::s_ShaderLib;


	Shader* Shader::Create(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
	{
		if (Exists(name))
			return Get(name);

		switch (Renderer2D::Get().GetAPI())
		{
		case GraphicsAPI::OpenGL:
			return new OpenGLShader(name, vertexSource, fragmentSource);
		case GraphicsAPI::Vulkan:
			HBL_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLShader(name, vertexSource, fragmentSource);
		case HBL::GraphicsAPI::None:
			HBL_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			return nullptr;
		}
	}

	Shader* Shader::Create(const std::string& name, const std::filesystem::path& vertexFilepath, const std::filesystem::path& fragmentFilepath)
	{
		if (Exists(name))
			return Get(name);

		// Parse vertex shader.
		std::fstream streamV(vertexFilepath);

		std::string line;
		std::stringstream ssV;

		while (getline(streamV, line))
		{
			ssV << line << '\n';
		}

		streamV.close();

		// Parse fragment shader.
		std::fstream streamF(fragmentFilepath);

		line.clear();
		std::stringstream ssF;

		while (getline(streamF, line))
		{
			ssF << line << '\n';
		}

		streamF.close();

		switch (Renderer2D::Get().GetAPI())
		{
		case GraphicsAPI::OpenGL:
			return new OpenGLShader(name, ssV.str(), ssF.str());
		case GraphicsAPI::Vulkan:
			HBL_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			return new OpenGLShader(name, ssV.str(), ssF.str());
		case HBL::GraphicsAPI::None:
			HBL_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			return nullptr;
		}
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

	Shader::Shader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
		: m_Name(name), m_VertexSource(vertexSource), m_FragmentSource(fragmentSource)
	{
	}
}