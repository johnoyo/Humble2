#pragma once

#include "Core/Context.h"

#include "Resources/ResourceManager.h"
#include "Script/BuildEngine.h"
#include "Utilities/YamlUtilities.h"

#include "imgui.h"

#include <iostream>
#include <regex>
#include <string>
#include <sstream>

namespace HBL2
{
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

		void DrawDefaultEditor(Reflect::Any& componentMeta)
		{
			Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (activeScene == nullptr)
			{
				return;
			}

			const Reflect::TypeEntry* typeEntry = Reflect::FindType(componentMeta.TypeId);

			if (typeEntry != nullptr)
			{
				struct FieldCallbackData
				{
					Scene* activeScene = nullptr;
					EditorUtilities* editorUtilities = nullptr;
				};

				FieldCallbackData fieldCallbackData =
				{
					.activeScene = activeScene,
					.editorUtilities = this,
				};

				// List all members.
				typeEntry->forEach(componentMeta.Ptr, &fieldCallbackData, [](void* userdata, std::string_view name, Reflect::Any value)
				{
					FieldCallbackData* fieldCallbackData = (FieldCallbackData*)userdata;
					fieldCallbackData->editorUtilities->DrawComponent(fieldCallbackData->activeScene, value, name.data());
				});
			}
		}

		void SerializeComponentToYAML(YAML::Emitter& out, Reflect::Any& componentMeta, Scene* ctx)
		{
			const Reflect::TypeEntry* typeEntry = Reflect::FindType(componentMeta.TypeId);

			if (typeEntry != nullptr)
			{
				struct FieldCallbackData
				{
					Scene* activeScene = nullptr;
					EditorUtilities* editorUtilities = nullptr;
					YAML::Emitter* out = nullptr;
				};

				FieldCallbackData fieldCallbackData =
				{
					.activeScene = ctx,
					.editorUtilities = this,
					.out = &out,
				};

				// List all members.
				typeEntry->forEach(componentMeta.Ptr, &fieldCallbackData, [](void* userdata, std::string_view name, Reflect::Any value)
				{
					FieldCallbackData* fieldCallbackData = (FieldCallbackData*)userdata;
					fieldCallbackData->editorUtilities->SerializeComponent(*fieldCallbackData->out, fieldCallbackData->activeScene, value, name.data());
				});
			}
		}

		void DeserializeComponentFromYAML(YAML::Node& node, Reflect::Any& componentMeta, Scene* ctx)
		{
			const Reflect::TypeEntry* typeEntry = Reflect::FindType(componentMeta.TypeId);

			if (typeEntry != nullptr)
			{
				struct FieldCallbackData
				{
					Scene* activeScene = nullptr;
					EditorUtilities* editorUtilities = nullptr;
					YAML::Node* node = nullptr;
				};

				FieldCallbackData fieldCallbackData =
				{
					.activeScene = ctx,
					.editorUtilities = this,
					.node = &node,
				};

				// List all members.
				typeEntry->forEach(componentMeta.Ptr, &fieldCallbackData, [](void* userdata, std::string_view name, Reflect::Any value)
				{
					FieldCallbackData* fieldCallbackData = (FieldCallbackData*)userdata;
					fieldCallbackData->editorUtilities->DeserializeComponent(*fieldCallbackData->node, fieldCallbackData->activeScene, value, name.data());
				});
			}
		}

		template<typename C>
		void DrawDefaultEditor(C& component)
		{
			Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (activeScene == nullptr)
			{
				return;
			}

			Reflect::Any componentMeta = Reflect::ForwardAsMeta(component);

			const Reflect::TypeEntry* typeEntry = Reflect::FindType(componentMeta.TypeId);

			if (typeEntry != nullptr)
			{
				struct FieldCallbackData
				{
					Scene* activeScene = nullptr;
					EditorUtilities* editorUtilities = nullptr;
					const Reflect::TypeEntry* typeEntry = nullptr;
				};

				FieldCallbackData fieldCallbackData =
				{
					.activeScene = activeScene,
					.editorUtilities = this,
					.typeEntry = typeEntry,
				};

				// List all members.
				typeEntry->forEach(componentMeta.Ptr, &fieldCallbackData, [](void* userdata, std::string_view name, Reflect::Any value)
				{
					FieldCallbackData* fieldCallbackData = (FieldCallbackData*)userdata;
					fieldCallbackData->editorUtilities->DrawComponent(fieldCallbackData->activeScene, value, fieldCallbackData->typeEntry, name.data());
				});
			}
		}

	private:
		void DrawComponent(Scene* ctx, Reflect::Any& fieldMeta, const char* memberName);
		void SerializeComponent(YAML::Emitter& out, Scene* ctx, Reflect::Any& fieldMeta, const char* memberName);
		void DeserializeComponent(YAML::Node& node, Scene* ctx, Reflect::Any& fieldMeta, const char* memberName);

	private:
		EditorUtilities() = default;
	};
}
