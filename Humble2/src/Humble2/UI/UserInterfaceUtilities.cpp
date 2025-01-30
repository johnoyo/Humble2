#include "UserInterfaceUtilities.h"

#include "Utilities\YamlUtilities.h"

namespace HBL2
{
	namespace UI
	{
		namespace Utils
		{
			ImVec2 GetViewportSize()
			{
				return *(ImVec2*)&Context::ViewportSize;
			}

			ImVec2 GetViewportPosition()
			{
				return *(ImVec2*)&Context::ViewportPosition;
			}

			float GetFontSize()
			{
				return ImGui::GetFontSize();
			}
		}
	}

	void RegisterComponentToReflection(const std::string& structCode)
	{
		// Regex pattern to match member declarations
		std::regex memberRegex(R"(([a-zA-Z_][a-zA-Z0-9_:<>]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)(?:\s*=\s*[^;]*)?;)");

		// Extract the struct name
		std::regex structNameRegex(R"(struct\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\{)");
		std::smatch match;

		std::string structName;
		if (std::regex_search(structCode, match, structNameRegex))
		{
			structName = match[1];
		}
		else
		{
			std::cerr << "Error: Could not extract struct name!" << std::endl;
			return;
		}

		// Begin generating reflection code
		std::ostringstream reflectionCode;
		reflectionCode << "entt::meta<" << structName << ">()\n";
		reflectionCode << "    .type(\"entt::hashed_string(typeid(" << structName << "\").name()))";

		// Match and process each member
		auto memberBegin = std::sregex_iterator(structCode.begin(), structCode.end(), memberRegex);
		auto memberEnd = std::sregex_iterator();

		for (auto it = memberBegin; it != memberEnd; ++it)
		{
			std::string type = (*it)[1].str();
			std::string name = (*it)[2].str();

			reflectionCode << "\n    .data<&" << structName << "::" << name << ">(\"" << name << "\"_hs).prop(\"name\"_hs, \"" << name << "\")";
		}

		reflectionCode << ";\n";

		// Output the generated code
		std::cout << "Generated Reflection Code:\n" << reflectionCode.str() << std::endl;
	}

	EditorUtilities& EditorUtilities::Get()
	{
		static EditorUtilities instance;
		return instance;
	}

	void EditorUtilities::DrawComponent(Scene* cxt, entt::meta_any& componentMeta, const char* typeName, const char* memberName)
	{
		auto data = entt::resolve(cxt->GetMetaContext(), entt::hashed_string(typeName)).data(entt::hashed_string(memberName));
		auto value = data.get(componentMeta);

		if (value)
		{
			if (value.type() == entt::resolve<glm::vec3>(cxt->GetMetaContext()))
			{
				glm::vec3* vec = value.try_cast<glm::vec3>();
				if (vec)
				{
					if (ImGui::DragFloat3(memberName, glm::value_ptr(*vec)))
					{
						data.set(componentMeta, *vec);
					}
				}
			}
			else if (value.type() == entt::resolve<glm::vec2>(cxt->GetMetaContext()))
			{
				glm::vec2* vec = value.try_cast<glm::vec2>();
				if (vec)
				{
					if (ImGui::DragFloat2(memberName, glm::value_ptr(*vec)))
					{
						data.set(componentMeta, *vec);
					}
				}
			}
			else if (value.type() == entt::resolve<float>(cxt->GetMetaContext()))
			{
				float* fpNumber = value.try_cast<float>();
				if (fpNumber)
				{
					if (ImGui::SliderFloat(memberName, fpNumber, 0, 150))
					{
						data.set(componentMeta, *fpNumber);
					}
				}
			}
			else if (value.type() == entt::resolve<double>(cxt->GetMetaContext()))
			{
				double* dpNumber = value.try_cast<double>();
				float* fpNumber = ((float*)dpNumber);

				if (fpNumber)
				{
					if (ImGui::SliderFloat(memberName, fpNumber, 0, 150))
					{
						data.set(componentMeta, *dpNumber);
					}
				}
			}
			else if (value.type() == entt::resolve<UUID>(cxt->GetMetaContext()))
			{
				UUID* scalar = value.try_cast<UUID>();
				if (scalar)
				{
					if (ImGui::InputScalar(memberName, ImGuiDataType_U32, scalar))
					{
						data.set(componentMeta, *scalar);
					}
				}
			}
			else if (value.type() == entt::resolve<int>(cxt->GetMetaContext()))
			{
				int* scalar = value.try_cast<int>();
				if (scalar)
				{
					if (ImGui::InputScalar(memberName, ImGuiDataType_U32, scalar))
					{
						data.set(componentMeta, *scalar);
					}
				}
			}
			else if (value.type() == entt::resolve<bool>(cxt->GetMetaContext()))
			{
				bool* flag = value.try_cast<bool>();
				if (flag)
				{
					if (ImGui::Checkbox(memberName, flag))
					{
						data.set(componentMeta, *flag);
					}
				}
			}
		}
		else
		{
			std::cerr << "Failed to retrieve member name!" << std::endl;
		}
	}
	
	void EditorUtilities::SerializeComponent(YAML::Emitter& out, Scene* cxt, entt::meta_any& componentMeta, const char* typeName, const char* memberName)
	{
		auto data = entt::resolve(cxt->GetMetaContext(), entt::hashed_string(typeName)).data(entt::hashed_string(memberName));
		auto value = data.get(componentMeta);

		if (value)
		{
			if (value.type() == entt::resolve<glm::vec3>(cxt->GetMetaContext()))
			{
				glm::vec3* vec = value.try_cast<glm::vec3>();
				if (vec)
				{
					out << YAML::Key << memberName << YAML::Value << *vec;
				}
			}
			else if (value.type() == entt::resolve<glm::vec2>(cxt->GetMetaContext()))
			{
				glm::vec2* vec = value.try_cast<glm::vec2>();
				if (vec)
				{
					out << YAML::Key << memberName << YAML::Value << *vec;
				}
			}
			else if (value.type() == entt::resolve<float>(cxt->GetMetaContext()))
			{
				float* fpNumber = value.try_cast<float>();
				if (fpNumber)
				{
					out << YAML::Key << memberName << YAML::Value << *fpNumber;
				}
			}
			else if (value.type() == entt::resolve<double>(cxt->GetMetaContext()))
			{
				double* dpNumber = value.try_cast<double>();
				float* fpNumber = ((float*)dpNumber);

				if (fpNumber)
				{
					out << YAML::Key << memberName << YAML::Value << *dpNumber;
				}
			}
			else if (value.type() == entt::resolve<UUID>(cxt->GetMetaContext()))
			{
				UUID* scalar = value.try_cast<UUID>();
				if (scalar)
				{
					out << YAML::Key << memberName << YAML::Value << *scalar;
				}
			}
			else if (value.type() == entt::resolve<int>(cxt->GetMetaContext()))
			{
				int* scalar = value.try_cast<int>();
				if (scalar)
				{
					out << YAML::Key << memberName << YAML::Value << *scalar;
				}
			}
			else if (value.type() == entt::resolve<bool>(cxt->GetMetaContext()))
			{
				bool* flag = value.try_cast<bool>();
				if (flag)
				{
					out << YAML::Key << memberName << YAML::Value << *flag;
				}
			}
		}
		else
		{
			std::cerr << "Failed to retrieve member name!" << std::endl;
		}
	}

	void EditorUtilities::DeserializeComponent(YAML::Node& node, Scene* cxt, entt::meta_any& componentMeta, const char* typeName, const char* memberName)
	{
		auto data = entt::resolve(cxt->GetMetaContext(), entt::hashed_string(typeName)).data(entt::hashed_string(memberName));
		auto value = data.get(componentMeta);

		if (value)
		{
			if (value.type() == entt::resolve<glm::vec3>(cxt->GetMetaContext()))
			{
				if (value.try_cast<glm::vec3>())
				{
					data.set(componentMeta, node[memberName].as<glm::vec3>());
				}
			}
			else if (value.type() == entt::resolve<glm::vec2>(cxt->GetMetaContext()))
			{
				if (value.try_cast<glm::vec2>())
				{
					data.set(componentMeta, node[memberName].as<glm::vec2>());
				}
			}
			else if (value.type() == entt::resolve<float>(cxt->GetMetaContext()))
			{
				if (value.try_cast<float>())
				{
					data.set(componentMeta, node[memberName].as<float>());
				}
			}
			else if (value.type() == entt::resolve<double>(cxt->GetMetaContext()))
			{
				if (value.try_cast<double>())
				{
					data.set(componentMeta, node[memberName].as<double>());
				}
			}
			else if (value.type() == entt::resolve<UUID>(cxt->GetMetaContext()))
			{
				if (value.try_cast<UUID>())
				{
					data.set(componentMeta, node[memberName].as<UUID>());
				}
			}
			else if (value.type() == entt::resolve<int>(cxt->GetMetaContext()))
			{
				if (value.try_cast<int>())
				{
					data.set(componentMeta, node[memberName].as<int>());
				}
			}
			else if (value.type() == entt::resolve<bool>(cxt->GetMetaContext()))
			{
				if (value.try_cast<bool>())
				{
					data.set(componentMeta, node[memberName].as<bool>());
				}
			}
		}
		else
		{
			std::cerr << "Failed to retrieve member name!" << std::endl;
		}
	}
}
