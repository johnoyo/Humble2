#include "UserInterfaceUtilities.h"

#include "Utilities\YamlUtilities.h"
#include "Resources\Types.h"

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

	EditorUtilities& EditorUtilities::Get()
	{
		static EditorUtilities instance;
		return instance;
	}

	void EditorUtilities::DrawComponent(Scene* ctx, Reflect::Any& fieldMeta, const char* memberName)
	{
		if (auto* v = fieldMeta.TryGetAs<glm::vec4>())
		{
			if (ImGui::DragFloat4(memberName, glm::value_ptr(*v)))
			{
				fieldMeta.Set(*v);
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<glm::vec3>())
		{
			if (ImGui::DragFloat3(memberName, glm::value_ptr(*v)))
			{
				fieldMeta.Set(*v);
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<glm::vec2>())
		{
			if (ImGui::DragFloat2(memberName, glm::value_ptr(*v)))
			{
				fieldMeta.Set(*v);
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<float>())
		{
			if (ImGui::SliderFloat(memberName, v, 0, 150))
			{
				fieldMeta.Set(*v);
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<double>())
		{
			if (ImGui::SliderFloat(memberName, (float*)v, 0, 150))
			{
				fieldMeta.Set(*v);
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<UUID>())
		{
			if (ImGui::InputScalar(memberName, ImGuiDataType_U64, v))
			{
				fieldMeta.Set(*v);
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<int>())
		{
			if (ImGui::InputScalar(memberName, ImGuiDataType_U32, v))
			{
				fieldMeta.Set(*v);
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<bool>())
		{
			if (ImGui::Checkbox(memberName, v))
			{
				fieldMeta.Set(*v);
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Scene>>())
		{
			uint32_t sceneHandlePacked = v->Pack();
			ImGui::InputScalar(memberName, ImGuiDataType_U32, (void*)(intptr_t*)&sceneHandlePacked);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Scene"))
				{
					uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
					Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

					if (assetHandle.IsValid())
					{
						Handle<Scene> sceneHandle = AssetManager::Instance->GetAsset<Scene>(assetHandle);
						fieldMeta.Set(sceneHandle);
					}
				}

				ImGui::EndDragDropTarget();
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Sound>>())
		{
			uint32_t soundHandlePacked = v->Pack();
			ImGui::InputScalar(memberName, ImGuiDataType_U32, (void*)(intptr_t*)&soundHandlePacked);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Sound"))
				{
					uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
					Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

					if (assetHandle.IsValid())
					{
						Handle<Sound> soundHandle = AssetManager::Instance->GetAsset<Sound>(assetHandle);
						fieldMeta.Set(soundHandle);
					}
				}

				ImGui::EndDragDropTarget();
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Shader>>())
		{
			uint32_t shaderHandlePacked = v->Pack();
			ImGui::InputScalar(memberName, ImGuiDataType_U32, (void*)(intptr_t*)&shaderHandlePacked);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Shader"))
				{
					uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
					Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

					if (assetHandle.IsValid())
					{
						Handle<Shader> shaderHandle = AssetManager::Instance->GetAsset<Shader>(assetHandle);
						fieldMeta.Set(shaderHandle);
					}
				}

				ImGui::EndDragDropTarget();
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Material>>())
		{
			uint32_t materialHandlePacked = v->Pack();
			ImGui::InputScalar(memberName, ImGuiDataType_U32, (void*)(intptr_t*)&materialHandlePacked);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Material"))
				{
					uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
					Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

					if (assetHandle.IsValid())
					{
						Handle<Material> materialHandle = AssetManager::Instance->GetAsset<Material>(assetHandle);
						fieldMeta.Set(materialHandle);
					}
				}

				ImGui::EndDragDropTarget();
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Mesh>>())
		{
			uint32_t meshHandlePacked = v->Pack();
			ImGui::InputScalar(memberName, ImGuiDataType_U32, (void*)(intptr_t*)&meshHandlePacked);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Mesh"))
				{
					uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
					Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

					if (assetHandle.IsValid())
					{
						Handle<Mesh> meshHandle = AssetManager::Instance->GetAsset<Mesh>(assetHandle);
						fieldMeta.Set(meshHandle);
					}
				}

				ImGui::EndDragDropTarget();
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Texture>>())
		{
			uint32_t textureHandlePacked = v->Pack();
			ImGui::InputScalar(memberName, ImGuiDataType_U32, (void*)(intptr_t*)&textureHandlePacked);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Texture"))
				{
					uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
					Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

					if (assetHandle.IsValid())
					{
						Handle<Texture> textureHandle = AssetManager::Instance->GetAsset<Texture>(assetHandle);
						fieldMeta.Set(textureHandle);
					}
				}

				ImGui::EndDragDropTarget();
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Prefab>>())
		{
			uint32_t prefabHandlePacked = v->Pack();
			ImGui::InputScalar(memberName, ImGuiDataType_U32, (void*)(intptr_t*)&prefabHandlePacked);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Prefab"))
				{
					uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
					Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

					if (assetHandle.IsValid())
					{
						Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(assetHandle);
						fieldMeta.Set(prefabHandle);
					}
				}

				ImGui::EndDragDropTarget();
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Entity>())
		{
			uint64_t packedEntity = v->Pack();
			ImGui::InputScalar(memberName, ImGuiDataType_S64, (void*)&packedEntity);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity_UUID"))
				{
					UUID entityUUID = *((UUID*)payload->Data);
					Entity retrievedEntity = ctx->FindEntityByUUID(entityUUID);
					fieldMeta.Set(retrievedEntity);
				}

				ImGui::EndDragDropTarget();
			}
		}
	}
	
	void EditorUtilities::SerializeComponent(YAML::Emitter& out, Scene* ctx, Reflect::Any& fieldMeta, const char* memberName)
	{
		if (auto* v = fieldMeta.TryGetAs<glm::vec4>())
		{
			out << YAML::Key << memberName << YAML::Value << *v;
		}
		else if (auto* v = fieldMeta.TryGetAs<glm::vec3>())
		{
			out << YAML::Key << memberName << YAML::Value << *v;
		}
		else if (auto* v = fieldMeta.TryGetAs<glm::vec2>())
		{
			out << YAML::Key << memberName << YAML::Value << *v;
		}
		else if (auto* v = fieldMeta.TryGetAs<float>())
		{
			out << YAML::Key << memberName << YAML::Value << *v;
		}
		else if (auto* v = fieldMeta.TryGetAs<double>())
		{
			out << YAML::Key << memberName << YAML::Value << *v;
		}
		else if (auto* v = fieldMeta.TryGetAs<UUID>())
		{
			out << YAML::Key << memberName << YAML::Value << *v;
		}
		else if (auto* v = fieldMeta.TryGetAs<int>())
		{
			out << YAML::Key << memberName << YAML::Value << *v;
		}
		else if (auto* v = fieldMeta.TryGetAs<bool>())
		{
			out << YAML::Key << memberName << YAML::Value << *v;
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Scene>>())
		{
			const auto& assetHandles = AssetManager::Instance->GetRegisteredAssets();

			Asset* asset = nullptr;

			for (auto handle : assetHandles)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Type == AssetType::Scene && asset->Indentifier != 0 && asset->Indentifier == v->Pack())
				{
					asset = asset;
					break;
				}
			}

			if (asset != nullptr)
			{
				out << YAML::Key << memberName << YAML::Value << asset->UUID;
			}
			else
			{
				out << YAML::Key << memberName << YAML::Value << (UUID)0;
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Sound>>())
		{
			const auto& assetHandles = AssetManager::Instance->GetRegisteredAssets();

			Asset* asset = nullptr;

			for (auto handle : assetHandles)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Type == AssetType::Sound && asset->Indentifier != 0 && asset->Indentifier == v->Pack())
				{
					asset = asset;
					break;
				}
			}

			if (asset != nullptr)
			{
				out << YAML::Key << memberName << YAML::Value << asset->UUID;
			}
			else
			{
				out << YAML::Key << memberName << YAML::Value << (UUID)0;
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Shader>>())
		{
			const auto& assetHandles = AssetManager::Instance->GetRegisteredAssets();

			Asset* asset = nullptr;

			for (auto handle : assetHandles)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Type == AssetType::Shader && asset->Indentifier != 0 && asset->Indentifier == v->Pack())
				{
					asset = asset;
					break;
				}
			}

			if (asset != nullptr)
			{
				out << YAML::Key << memberName << YAML::Value << asset->UUID;
			}
			else
			{
				out << YAML::Key << memberName << YAML::Value << (UUID)0;
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Material>>())
		{
			const auto& assetHandles = AssetManager::Instance->GetRegisteredAssets();

			Asset* asset = nullptr;

			for (auto handle : assetHandles)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Type == AssetType::Material && asset->Indentifier != 0 && asset->Indentifier == v->Pack())
				{
					asset = asset;
					break;
				}
			}

			if (asset != nullptr)
			{
				out << YAML::Key << memberName << YAML::Value << asset->UUID;
			}
			else
			{
				out << YAML::Key << memberName << YAML::Value << (UUID)0;
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Mesh>>())
		{
			const auto& assetHandles = AssetManager::Instance->GetRegisteredAssets();

			Asset* asset = nullptr;

			for (auto handle : assetHandles)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Type == AssetType::Mesh && asset->Indentifier != 0 && asset->Indentifier == v->Pack())
				{
					asset = asset;
					break;
				}
			}

			if (asset != nullptr)
			{
				out << YAML::Key << memberName << YAML::Value << asset->UUID;
			}
			else
			{
				out << YAML::Key << memberName << YAML::Value << (UUID)0;
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Texture>>())
		{
			const auto& assetHandles = AssetManager::Instance->GetRegisteredAssets();

			Asset* asset = nullptr;

			for (auto handle : assetHandles)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Type == AssetType::Texture && asset->Indentifier != 0 && asset->Indentifier == v->Pack())
				{
					asset = asset;
					break;
				}
			}

			if (asset != nullptr)
			{
				out << YAML::Key << memberName << YAML::Value << asset->UUID;
			}
			else
			{
				out << YAML::Key << memberName << YAML::Value << (UUID)0;
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Prefab>>())
		{
			const auto& assetHandles = AssetManager::Instance->GetRegisteredAssets();

			Asset* asset = nullptr;

			for (auto handle : assetHandles)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Type == AssetType::Prefab && asset->Indentifier != 0 && asset->Indentifier == v->Pack())
				{
					asset = asset;
					break;
				}
			}

			if (asset != nullptr)
			{
				out << YAML::Key << memberName << YAML::Value << asset->UUID;
			}
			else
			{
				out << YAML::Key << memberName << YAML::Value << (UUID)0;
			}
		}
		else if (auto* v = fieldMeta.TryGetAs<Entity>())
		{
			auto* id = ctx->TryGetComponent<Component::ID>(*v);
			out << YAML::Key << memberName << YAML::Value << id->Identifier;
		}
	}

	void EditorUtilities::DeserializeComponent(YAML::Node& node, Scene* ctx, Reflect::Any& fieldMeta, const char* memberName)
	{
		if (!node[memberName].IsDefined())
		{
			return;
		}

		if (auto* v = fieldMeta.TryGetAs<glm::vec4>())
		{
			const auto& vr = node[memberName].as<glm::vec4>();
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<glm::vec3>())
		{
			const auto& vr = node[memberName].as<glm::vec3>();
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<glm::vec2>())
		{
			const auto& vr = node[memberName].as<glm::vec2>();
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<float>())
		{
			auto vr = node[memberName].as<float>();
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<double>())
		{
			auto vr = node[memberName].as<double>();
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<UUID>())
		{
			auto vr = node[memberName].as<UUID>();
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<int>())
		{
			auto vr = node[memberName].as<int>();
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<bool>())
		{
			auto vr = node[memberName].as<bool>();
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Scene>>())
		{
			auto vr = AssetManager::Instance->GetAsset<Scene>(node[memberName].as<UUID>());
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Sound>>())
		{
			auto vr = AssetManager::Instance->GetAsset<Sound>(node[memberName].as<UUID>());
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Shader>>())
		{
			auto vr = AssetManager::Instance->GetAsset<Shader>(node[memberName].as<UUID>());
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Material>>())
		{
			auto vr = AssetManager::Instance->GetAsset<Material>(node[memberName].as<UUID>());
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Mesh>>())
		{
			auto vr = AssetManager::Instance->GetAsset<Mesh>(node[memberName].as<UUID>());
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Texture>>())
		{
			auto vr = AssetManager::Instance->GetAsset<Texture>(node[memberName].as<UUID>());
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<Handle<Prefab>>())
		{
			auto vr = AssetManager::Instance->GetAsset<Prefab>(node[memberName].as<UUID>());
			fieldMeta.Set(vr);
		}
		else if (auto* v = fieldMeta.TryGetAs<Entity>())
		{
			auto vr = ctx->FindEntityByUUID(node[memberName].as<UUID>());
			fieldMeta.Set(vr);
		}
	}
}
