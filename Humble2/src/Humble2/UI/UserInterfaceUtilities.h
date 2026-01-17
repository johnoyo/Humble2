#pragma once

#include "Core\Context.h"
#include "EditorInspector.h"

#include "Resources/ResourceManager.h"
#include "Script/BuildEngine.h"
#include "Utilities\YamlUtilities.h"

#include "entt.hpp"
#include "imgui.h"

#include <iostream>
#include <regex>
#include <string>
#include <sstream>

namespace HBL2
{
	struct MemberNameTag
	{
		std::string Value;
	};

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
				if (auto* name = static_cast<MemberNameTag*>(data.custom()))
				{
					std::string typeName = componentMeta.type().info().name().data();

					typeName = BuildEngine::Instance->CleanComponentNameO1(typeName);

					const char* typeNameClean = typeName.c_str();
					const char* memberName = name->Value.c_str();

					DrawComponent(activeScene, componentMeta, typeNameClean, memberName);
				}
			}			
		}

		void SerializeComponentToYAML(YAML::Emitter& out, entt::meta_any& componentMeta, Scene* ctx)
		{
			using namespace entt::literals;

			// List all members
			for (auto [id, data] : entt::resolve(ctx->GetMetaContext(), componentMeta.type().info().hash()).data())
			{
				// Retrieve the name property of the member
				if (auto* name = static_cast<MemberNameTag*>(data.custom()))
				{
					std::string typeName = componentMeta.type().info().name().data();

					typeName = BuildEngine::Instance->CleanComponentNameO1(typeName);

					const char* typeNameClean = typeName.c_str();
					const char* memberName = name->Value.c_str();

					SerializeComponent(out, ctx, componentMeta, typeNameClean, memberName);
				}
			}
		}

		void DeserializeComponentFromYAML(YAML::Node& node, entt::meta_any& componentMeta, Scene* ctx)
		{
			using namespace entt::literals;

			// List all members
			for (auto [id, data] : entt::resolve(ctx->GetMetaContext(), componentMeta.type().info().hash()).data())
			{
				// Retrieve the name property of the member
				if (auto* name = static_cast<MemberNameTag*>(data.custom()))
				{
					std::string typeName = componentMeta.type().info().name().data();

					typeName = BuildEngine::Instance->CleanComponentNameO1(typeName);

					const char* typeNameClean = typeName.c_str();
					const char* memberName = name->Value.c_str();

					DeserializeComponent(node, ctx, componentMeta, typeNameClean, memberName);
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
				if (auto* name = static_cast<MemberNameTag*>(data.custom()))
				{
					const char* typeName = typeid(C).name();
					const char* memberName = name->Value.c_str();

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
		void DrawComponent(Scene* ctx, entt::meta_any& componentMeta, const char* typeName, const char* memberName);

		void SerializeComponent(YAML::Emitter& out, Scene* ctx, entt::meta_any& componentMeta, const char* typeName, const char* memberName);

		void DeserializeComponent(YAML::Node& node, Scene* ctx, entt::meta_any& componentMeta, const char* typeName, const char* memberName);

	private:
		EditorUtilities() = default;

		std::unordered_map<size_t, void*> m_CustomEditors;
	};
}