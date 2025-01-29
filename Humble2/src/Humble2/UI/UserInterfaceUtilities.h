#pragma once

#include "Core\Context.h"
#include "Editor.h"

#include "Resources/ResourceManager.h"
#include "Utilities/NativeScriptUtilities.h"

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

		void DrawDefaultEditor(entt::meta_any& componentMeta)
		{
			using namespace entt::literals;

			Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			// List all members
			for (auto [id, data] : entt::resolve(activeScene->GetMetaContext(), componentMeta.type().info().hash()).data())
			{
				// Retrieve the name property of the member
				auto name_prop = data.prop("name"_hs);
				if (name_prop)
				{
					std::string typeName = componentMeta.type().info().name().data();

					typeName = NativeScriptUtilities::Get().CleanComponentNameO1(typeName);

					const char* typeNameClean = typeName.c_str();
					const char* memberName = name_prop.value().cast<const char*>();

					DrawComponent(activeScene, componentMeta, typeNameClean, memberName);
				}
			}
			
		}

		template<typename C>
		void DrawDefaultEditor(C& component)
		{
			using namespace entt::literals;

			Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			entt::meta_any componentMeta = entt::forward_as_meta(activeScene->GetMetaContext(), component);

			// List all members
			for (auto [id, data] : entt::resolve<C>(activeScene->GetMetaContext()).data())
			{
				// Retrieve the name property of the member
				auto name_prop = data.prop("name"_hs);
				if (name_prop)
				{
					const char* typeName = typeid(C).name();
					const char* memberName = name_prop.value().cast<const char*>();

					DrawComponent(activeScene, componentMeta, typeName, memberName);
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
		void DrawComponent(Scene* cxt, entt::meta_any& componentMeta, const char* typeName, const char* memberName)
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

	private:
		EditorUtilities() = default;

		std::unordered_map<size_t, void*> m_CustomEditors;
	};
}