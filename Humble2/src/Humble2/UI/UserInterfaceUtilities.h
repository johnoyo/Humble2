#pragma once

#include "Editor.h"

#include "Vendor/entt/include/entt.hpp"
#include "imgui.h"

#include <iostream>
#include <regex>
#include <string>
#include <sstream>

namespace HBL2
{
	static inline void RegisterComponentToReflection(const std::string& structCode)
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

	namespace UI
	{
		namespace Utils
		{
			static inline ImVec2 GetViewportSize()
			{
				return *(ImVec2*)&Context::ViewportSize;
			}

			static inline ImVec2 GetViewportPosition()
			{
				return *(ImVec2*)&Context::ViewportPosition;
			}

			static inline float GetFontSize()
			{
				return ImGui::GetFontSize();
			}
		}
	}

	class EditorUtilities
	{
	public:
		EditorUtilities(const EditorUtilities&) = delete;

		static EditorUtilities& Get()
		{
			static EditorUtilities instance;
			return instance;
		}

		template<typename C>
		void DrawDefaultEditor(C& component)
		{
			using namespace entt::literals;

			entt::meta_any componentMeta = entt::forward_as_meta(component);

			using namespace entt::literals;

			// List all members
			for (auto [id, data] : entt::resolve<C>().data())
			{
				// Retrieve the name property of the member
				auto name_prop = data.prop("name"_hs);
				if (name_prop)
				{
					const char* typeName = typeid(C).name();
					const char* memberName = name_prop.value().cast<const char*>();

					auto data = entt::resolve(entt::hashed_string(typeName)).data(entt::hashed_string(memberName));
					auto value = data.get(componentMeta);

					if (value)
					{
						if (value.type() == entt::resolve<glm::vec3>())
						{
							glm::vec3* vec = value.try_cast<glm::vec3>();
							if (vec)
							{
								ImGui::DragFloat3(memberName, glm::value_ptr(*vec));
							}

							data.set(componentMeta, vec);
						}
						else if (value.type() == entt::resolve<glm::vec2>())
						{
							glm::vec2* vec = value.try_cast<glm::vec2>();
							if (vec)
							{
								ImGui::DragFloat2(memberName, glm::value_ptr(*vec));
							}

							data.set(componentMeta, vec);
						}
						else if (value.type() == entt::resolve<float>())
						{
							float* fpNumber = value.try_cast<float>();
							if (fpNumber)
							{
								ImGui::SliderFloat(memberName, fpNumber, 0, 150);
							}

							data.set(componentMeta, *fpNumber);
						}
						else if (value.type() == entt::resolve<double>())
						{
							double* dpNumber = value.try_cast<double>();
							float* fpNumber = ((float*)dpNumber);

							if (fpNumber)
							{
								ImGui::SliderFloat(memberName, fpNumber, 0, 150);
							}
							data.set(componentMeta, *dpNumber);
						}
						else if (value.type() == entt::resolve<UUID>())
						{
							UUID* scalar = value.try_cast<UUID>();
							if (scalar)
							{
								ImGui::InputScalar(memberName, ImGuiDataType_U32, scalar);
							}

							data.set(componentMeta, *scalar);
						}
						else if (value.type() == entt::resolve<bool>())
						{
							bool* flag = value.try_cast<bool>();
							if (flag)
							{
								ImGui::Checkbox(memberName, flag);
							}

							data.set(componentMeta, *flag);
						}
					}
				}
				else
				{
					std::cerr << "Failed to retrieve member name!" << std::endl;
				}
			}
		}

		template<typename C>
		bool HasCustomEditor()
		{
			return m_CustomEditors.find(typeid(C).hash_code()) != m_CustomEditors.end();
		}

		template<typename C, typename E>
		bool DrawCustomEditor(const C& component)
		{
			E* customEditor = (E*)m_CustomEditors[typeid(C).hash_code()];
			customEditor->OnUpdate(component);
			return customEditor->GetRenderBaseEditor();
		}

		template<typename C, typename E>
		void InitCustomEditor()
		{
			E* customEditor = (E*)m_CustomEditors[typeid(C).hash_code()];
			customEditor->OnCreate();
		}

		template<typename C, typename E>
		void RegisterCustomEditor()
		{
			m_CustomEditors[typeid(C).hash_code()] = new E;
		}

	private:
		EditorUtilities() = default;

		std::unordered_map<size_t, void*> m_CustomEditors;
	};
}