#pragma once

#include "Core\Context.h"
#include "Editor.h"

#include "Vendor/entt/include/entt.hpp"
#include "imgui.h"

#include <iostream>
#include <regex>
#include <string>
#include <sstream>

namespace HBL2
{
	HBL2_API void RegisterComponentToReflection(const std::string& structCode);

	namespace UI
	{
		namespace Utils
		{
			HBL2_API ImVec2 GetViewportSize();

			HBL2_API ImVec2 GetViewportPosition();

			HBL2_API float GetFontSize();
		}
	}

	class HBL2_API EditorUtilities
	{
	public:
		EditorUtilities(const EditorUtilities&) = delete;

		static EditorUtilities& Get();

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