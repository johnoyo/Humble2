#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>

namespace HBL
{
	class Shader
	{
	public:
		static Shader* Create(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
		static Shader* Create(const std::string& name, const std::filesystem::path& vertexFilepath, const std::filesystem::path& fragmentFilepath);

		static void Add(const std::string& name, Shader* shader);
		static Shader* Shader::Get(const std::string& name);
		static bool Shader::Exists(const std::string& name);
		static void Clean();

		Shader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

	protected:
		uint32_t m_ID;
		std::string m_Name;
		std::string m_VertexSource;
		std::string m_FragmentSource;
	private:
		static std::unordered_map<std::string, Shader*> s_ShaderLib;
	};
}