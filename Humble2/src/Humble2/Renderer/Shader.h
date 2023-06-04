#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image/stb_image.h>

#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>

namespace HBL2
{
	class Shader
	{
	public:
		Shader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
		virtual ~Shader() = default;

		static Shader* Create(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource, int tmp);
		static Shader* Create(const std::string& name, const std::string& vertexFilepath, const std::string& fragmentFilepath);
		static Shader* Create(const std::string& name, const std::string& filepath);

		static void Add(const std::string& name, Shader* shader);
		static Shader* Get(const std::string& name);
		static bool Exists(const std::string& name);
		static void Clean();

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		virtual void SetFloat2(const glm::vec2& vec2, const std::string& uniformName) = 0;
		virtual void SetMat4(const glm::mat4& mat4, const std::string& uniformName) = 0;
		virtual void SetInt1(const int32_t value, const std::string& uniformName) = 0;
		virtual void SetIntPtr1(const int32_t* value, int count, const std::string& uniformName) = 0;

	protected:
		uint32_t m_ID;
		std::string m_Name;
		std::string m_VertexSource;
		std::string m_FragmentSource;
	private:
		static std::unordered_map<std::string, Shader*> s_ShaderLib;
	};
}