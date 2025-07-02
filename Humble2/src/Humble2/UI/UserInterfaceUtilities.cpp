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

	void EditorUtilities::DrawComponent(Scene* ctx, entt::meta_any& componentMeta, const char* typeName, const char* memberName)
	{
		auto data = entt::resolve(ctx->GetMetaContext(), entt::hashed_string(typeName)).data(entt::hashed_string(memberName));
		auto value = data.get(componentMeta);

		if (value)
		{
			if (value.type() == entt::resolve<glm::vec4>(ctx->GetMetaContext()))
			{
				glm::vec4* vec = value.try_cast<glm::vec4>();
				if (vec)
				{
					if (ImGui::DragFloat4(memberName, glm::value_ptr(*vec)))
					{
						data.set(componentMeta, *vec);
					}
				}
			}
			else if (value.type() == entt::resolve<glm::vec3>(ctx->GetMetaContext()))
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
			else if (value.type() == entt::resolve<glm::vec2>(ctx->GetMetaContext()))
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
			else if (value.type() == entt::resolve<float>(ctx->GetMetaContext()))
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
			else if (value.type() == entt::resolve<double>(ctx->GetMetaContext()))
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
			else if (value.type() == entt::resolve<UUID>(ctx->GetMetaContext()))
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
			else if (value.type() == entt::resolve<int>(ctx->GetMetaContext()))
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
			else if (value.type() == entt::resolve<bool>(ctx->GetMetaContext()))
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
			else if (value.type() == entt::resolve<Handle<Scene>>(ctx->GetMetaContext()))
			{
				Handle<Scene>* sceneHandle = value.try_cast<Handle<Scene>>();
				if (sceneHandle)
				{
					uint32_t sceneHandlePacked = sceneHandle->Pack();
					ImGui::InputScalar(memberName, ImGuiDataType_U32, (void*)(intptr_t*)&sceneHandlePacked);
					
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Scene"))
						{
							uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
							Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

							if (assetHandle.IsValid())
							{
								data.set(componentMeta, AssetManager::Instance->GetAsset<Scene>(assetHandle));
							}

							ImGui::EndDragDropTarget();
						}
					}					
				}
			}
			else if (value.type() == entt::resolve<Handle<Sound>>(ctx->GetMetaContext()))
			{
				Handle<Sound>* soundHandle = value.try_cast<Handle<Sound>>();
				if (soundHandle)
				{
					uint32_t soundHandlePacked = soundHandle->Pack();
					ImGui::InputScalar(memberName, ImGuiDataType_U32, (void*)(intptr_t*)&soundHandlePacked);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Sound"))
						{
							uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
							Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

							if (assetHandle.IsValid())
							{
								data.set(componentMeta, AssetManager::Instance->GetAsset<Sound>(assetHandle));
							}

							ImGui::EndDragDropTarget();
						}
					}
				}
			}
			else if (value.type() == entt::resolve<Handle<Shader>>(ctx->GetMetaContext()))
			{
				Handle<Shader>* shaderHandle = value.try_cast<Handle<Shader>>();
				if (shaderHandle)
				{
					uint32_t shaderHandlePacked = shaderHandle->Pack();
					ImGui::InputScalar(memberName, ImGuiDataType_U32, (void*)(intptr_t*)&shaderHandlePacked);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Shader"))
						{
							uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
							Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

							if (assetHandle.IsValid())
							{
								data.set(componentMeta, AssetManager::Instance->GetAsset<Shader>(assetHandle));
							}

							ImGui::EndDragDropTarget();
						}
					}
				}
			}
			else if (value.type() == entt::resolve<Handle<Material>>(ctx->GetMetaContext()))
			{
				Handle<Material>* materialHandle = value.try_cast<Handle<Material>>();
				if (materialHandle)
				{
					uint32_t materialHandlePacked = materialHandle->Pack();
					ImGui::InputScalar(memberName, ImGuiDataType_U32, (void*)(intptr_t*)&materialHandlePacked);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Material"))
						{
							uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
							Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

							if (assetHandle.IsValid())
							{
								data.set(componentMeta, AssetManager::Instance->GetAsset<Material>(assetHandle));
							}

							ImGui::EndDragDropTarget();
						}
					}
				}
			}
			else if (value.type() == entt::resolve<Handle<Mesh>>(ctx->GetMetaContext()))
			{
				Handle<Mesh>* meshHandle = value.try_cast<Handle<Mesh>>();
				if (meshHandle)
				{
					uint32_t meshHandlePacked = meshHandle->Pack();
					ImGui::InputScalar(memberName, ImGuiDataType_U32, (void*)(intptr_t*)&meshHandlePacked);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Mesh"))
						{
							uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
							Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

							if (assetHandle.IsValid())
							{
								data.set(componentMeta, AssetManager::Instance->GetAsset<Mesh>(assetHandle));
							}

							ImGui::EndDragDropTarget();
						}
					}
				}
			}
			else if (value.type() == entt::resolve<Handle<Texture>>(ctx->GetMetaContext()))
			{
				Handle<Texture>* textureHandle = value.try_cast<Handle<Texture>>();
				if (textureHandle)
				{
					uint32_t textureHandlePacked = textureHandle->Pack();
					ImGui::InputScalar(memberName, ImGuiDataType_U32, (void*)(intptr_t*)&textureHandlePacked);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Texture"))
						{
							uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
							Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

							if (assetHandle.IsValid())
							{
								data.set(componentMeta, AssetManager::Instance->GetAsset<Texture>(assetHandle));
							}

							ImGui::EndDragDropTarget();
						}
					}
				}
			}
			else if (value.type() == entt::resolve<entt::entity>(ctx->GetMetaContext()))
			{
				entt::entity* entity = value.try_cast<entt::entity>();
				if (entity)
				{
					ImGui::InputScalar(memberName, ImGuiDataType_U32, entity);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity_UUID"))
						{
							UUID entityUUID = *((UUID*)payload->Data);
							entt::entity retrievedEntity = ctx->FindEntityByUUID(entityUUID);

							data.set(componentMeta, retrievedEntity);
							ImGui::EndDragDropTarget();
						}
					}
				}
			}
			else if (value.type() == entt::resolve<Entity>(ctx->GetMetaContext()))
			{
				Entity* entity = value.try_cast<Entity>();
				if (entity)
				{
					ImGui::InputScalar(memberName, ImGuiDataType_U32, entity);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity_UUID"))
						{
							UUID entityUUID = *((UUID*)payload->Data);
							Entity retrievedEntity = ctx->FindEntityByUUID(entityUUID);

							data.set(componentMeta, retrievedEntity);
							ImGui::EndDragDropTarget();
						}
					}
				}
			}
		}
		else
		{
			std::cerr << "Failed to retrieve member name!" << std::endl;
		}
	}
	
	void EditorUtilities::SerializeComponent(YAML::Emitter& out, Scene* ctx, entt::meta_any& componentMeta, const char* typeName, const char* memberName)
	{
		auto data = entt::resolve(ctx->GetMetaContext(), entt::hashed_string(typeName)).data(entt::hashed_string(memberName));
		auto value = data.get(componentMeta);

		if (value)
		{
			if (value.type() == entt::resolve<glm::vec4>(ctx->GetMetaContext()))
			{
				glm::vec4* vec = value.try_cast<glm::vec4>();
				if (vec)
				{
					out << YAML::Key << memberName << YAML::Value << *vec;
				}
			}
			else if (value.type() == entt::resolve<glm::vec3>(ctx->GetMetaContext()))
			{
				glm::vec3* vec = value.try_cast<glm::vec3>();
				if (vec)
				{
					out << YAML::Key << memberName << YAML::Value << *vec;
				}
			}
			else if (value.type() == entt::resolve<glm::vec2>(ctx->GetMetaContext()))
			{
				glm::vec2* vec = value.try_cast<glm::vec2>();
				if (vec)
				{
					out << YAML::Key << memberName << YAML::Value << *vec;
				}
			}
			else if (value.type() == entt::resolve<float>(ctx->GetMetaContext()))
			{
				float* fpNumber = value.try_cast<float>();
				if (fpNumber)
				{
					out << YAML::Key << memberName << YAML::Value << *fpNumber;
				}
			}
			else if (value.type() == entt::resolve<double>(ctx->GetMetaContext()))
			{
				double* dpNumber = value.try_cast<double>();
				float* fpNumber = ((float*)dpNumber);

				if (fpNumber)
				{
					out << YAML::Key << memberName << YAML::Value << *dpNumber;
				}
			}
			else if (value.type() == entt::resolve<UUID>(ctx->GetMetaContext()))
			{
				UUID* scalar = value.try_cast<UUID>();
				if (scalar)
				{
					out << YAML::Key << memberName << YAML::Value << *scalar;
				}
			}
			else if (value.type() == entt::resolve<int>(ctx->GetMetaContext()))
			{
				int* scalar = value.try_cast<int>();
				if (scalar)
				{
					out << YAML::Key << memberName << YAML::Value << *scalar;
				}
			}
			else if (value.type() == entt::resolve<bool>(ctx->GetMetaContext()))
			{
				bool* flag = value.try_cast<bool>();
				if (flag)
				{
					out << YAML::Key << memberName << YAML::Value << *flag;
				}
			}
			else if (value.type() == entt::resolve<Handle<Scene>>(ctx->GetMetaContext()))
			{
				Handle<Scene>* sceneHandle = value.try_cast<Handle<Scene>>();
				if (sceneHandle)
				{
					const auto& assetHandles = AssetManager::Instance->GetRegisteredAssets();

					Asset* sceneAsset = nullptr;

					for (auto handle : assetHandles)
					{
						Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
						if (asset->Type == AssetType::Scene && asset->Indentifier != 0 && asset->Indentifier == sceneHandle->Pack())
						{
							sceneAsset = asset;
							break;
						}
					}

					if (sceneAsset != nullptr)
					{
						out << YAML::Key << memberName << YAML::Value << sceneAsset->UUID;
					}
					else
					{
						out << YAML::Key << memberName << YAML::Value << (UUID)0;
					}
				}
			}
			else if (value.type() == entt::resolve<Handle<Sound>>(ctx->GetMetaContext()))
			{
				Handle<Sound>* soundHandle = value.try_cast<Handle<Sound>>();
				if (soundHandle)
				{
					const auto& assetHandles = AssetManager::Instance->GetRegisteredAssets();

					Asset* soundAsset = nullptr;

					for (auto handle : assetHandles)
					{
						Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
						if (asset->Type == AssetType::Sound && asset->Indentifier != 0 && asset->Indentifier == soundHandle->Pack())
						{
							soundAsset = asset;
							break;
						}
					}

					if (soundAsset != nullptr)
					{
						out << YAML::Key << memberName << YAML::Value << soundAsset->UUID;
					}
					else
					{
						out << YAML::Key << memberName << YAML::Value << (UUID)0;
					}
				}
			}
			else if (value.type() == entt::resolve<Handle<Shader>>(ctx->GetMetaContext()))
			{
				Handle<Shader>* shaderHandle = value.try_cast<Handle<Shader>>();
				if (shaderHandle)
				{
					const auto& assetHandles = AssetManager::Instance->GetRegisteredAssets();

					Asset* shaderAsset = nullptr;

					for (auto handle : assetHandles)
					{
						Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
						if (asset->Type == AssetType::Shader && asset->Indentifier != 0 && asset->Indentifier == shaderHandle->Pack())
						{
							shaderAsset = asset;
							break;
						}
					}

					if (shaderAsset != nullptr)
					{
						out << YAML::Key << memberName << YAML::Value << shaderAsset->UUID;
					}
					else
					{
						out << YAML::Key << memberName << YAML::Value << (UUID)0;
					}
				}
			}
			else if (value.type() == entt::resolve<Handle<Material>>(ctx->GetMetaContext()))
			{
				Handle<Material>* materialHandle = value.try_cast<Handle<Material>>();
				if (materialHandle)
				{
					const auto& assetHandles = AssetManager::Instance->GetRegisteredAssets();

					Asset* materialAsset = nullptr;

					for (auto handle : assetHandles)
					{
						Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
						if (asset->Type == AssetType::Material && asset->Indentifier != 0 && asset->Indentifier == materialHandle->Pack())
						{
							materialAsset = asset;
							break;
						}
					}

					if (materialAsset != nullptr)
					{
						out << YAML::Key << memberName << YAML::Value << materialAsset->UUID;
					}
					else
					{
						out << YAML::Key << memberName << YAML::Value << (UUID)0;
					}
				}
			}
			else if (value.type() == entt::resolve<Handle<Mesh>>(ctx->GetMetaContext()))
			{
				Handle<Mesh>* meshHandle = value.try_cast<Handle<Mesh>>();
				if (meshHandle)
				{
					const auto& assetHandles = AssetManager::Instance->GetRegisteredAssets();

					Asset* meshAsset = nullptr;

					for (auto handle : assetHandles)
					{
						Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
						if (asset->Type == AssetType::Mesh && asset->Indentifier != 0 && asset->Indentifier == meshHandle->Pack())
						{
							meshAsset = asset;
							break;
						}
					}

					if (meshAsset != nullptr)
					{
						out << YAML::Key << memberName << YAML::Value << meshAsset->UUID;
					}
					else
					{
						out << YAML::Key << memberName << YAML::Value << (UUID)0;
					}
				}
			}
			else if (value.type() == entt::resolve<Handle<Texture>>(ctx->GetMetaContext()))
			{
				Handle<Texture>* textureHandle = value.try_cast<Handle<Texture>>();
				if (textureHandle)
				{
					const auto& assetHandles = AssetManager::Instance->GetRegisteredAssets();

					Asset* textureAsset = nullptr;

					for (auto handle : assetHandles)
					{
						Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
						if (asset->Type == AssetType::Texture && asset->Indentifier != 0 && asset->Indentifier == textureHandle->Pack())
						{
							textureAsset = asset;
							break;
						}
					}

					if (textureAsset != nullptr)
					{
						out << YAML::Key << memberName << YAML::Value << textureAsset->UUID;
					}
					else
					{
						out << YAML::Key << memberName << YAML::Value << (UUID)0;
					}
				}
			}
			else if (value.type() == entt::resolve<entt::entity>(ctx->GetMetaContext()))
			{
				entt::entity* entity = value.try_cast<entt::entity>();
				if (entity)
				{
					auto* id = ctx->TryGetComponent<Component::ID>(*entity);
					if (id)
					{
						out << YAML::Key << memberName << YAML::Value << id->Identifier;
					}
				}
			}
			else if (value.type() == entt::resolve<Entity>(ctx->GetMetaContext()))
			{
				Entity* entity = value.try_cast<Entity>();
				if (entity)
				{
					auto* id = ctx->TryGetComponent<Component::ID>(*entity);
					if (id)
					{
						out << YAML::Key << memberName << YAML::Value << id->Identifier;
					}
				}
			}
		}
		else
		{
			std::cerr << "Failed to retrieve member name!" << std::endl;
		}
	}

	void EditorUtilities::DeserializeComponent(YAML::Node& node, Scene* ctx, entt::meta_any& componentMeta, const char* typeName, const char* memberName)
	{
		auto data = entt::resolve(ctx->GetMetaContext(), entt::hashed_string(typeName)).data(entt::hashed_string(memberName));
		auto value = data.get(componentMeta);

		if (!node[memberName].IsDefined())
		{
			return;
		}

		if (value)
		{
			if (value.type() == entt::resolve<glm::vec4>(ctx->GetMetaContext()))
			{
				if (value.try_cast<glm::vec4>())
				{
					data.set(componentMeta, node[memberName].as<glm::vec4>());
				}
			}
			else if (value.type() == entt::resolve<glm::vec3>(ctx->GetMetaContext()))
			{
				if (value.try_cast<glm::vec3>())
				{
					data.set(componentMeta, node[memberName].as<glm::vec3>());
				}
			}
			else if (value.type() == entt::resolve<glm::vec2>(ctx->GetMetaContext()))
			{
				if (value.try_cast<glm::vec2>())
				{
					data.set(componentMeta, node[memberName].as<glm::vec2>());
				}
			}
			else if (value.type() == entt::resolve<float>(ctx->GetMetaContext()))
			{
				if (value.try_cast<float>())
				{
					data.set(componentMeta, node[memberName].as<float>());
				}
			}
			else if (value.type() == entt::resolve<double>(ctx->GetMetaContext()))
			{
				if (value.try_cast<double>())
				{
					data.set(componentMeta, node[memberName].as<double>());
				}
			}
			else if (value.type() == entt::resolve<UUID>(ctx->GetMetaContext()))
			{
				if (value.try_cast<UUID>())
				{
					data.set(componentMeta, node[memberName].as<UUID>());
				}
			}
			else if (value.type() == entt::resolve<int>(ctx->GetMetaContext()))
			{
				if (value.try_cast<int>())
				{
					data.set(componentMeta, node[memberName].as<int>());
				}
			}
			else if (value.type() == entt::resolve<bool>(ctx->GetMetaContext()))
			{
				if (value.try_cast<bool>())
				{
					data.set(componentMeta, node[memberName].as<bool>());
				}
			}
			else if (value.type() == entt::resolve<Handle<Scene>>(ctx->GetMetaContext()))
			{
				if (value.try_cast<Handle<Scene>>())
				{
					data.set(componentMeta, AssetManager::Instance->GetAsset<Scene>(node[memberName].as<UUID>()));
				}
			}
			else if (value.type() == entt::resolve<Handle<Sound>>(ctx->GetMetaContext()))
			{
				if (value.try_cast<Handle<Sound>>())
				{
					data.set(componentMeta, AssetManager::Instance->GetAsset<Sound>(node[memberName].as<UUID>()));
				}
			}
			else if (value.type() == entt::resolve<Handle<Shader>>(ctx->GetMetaContext()))
			{
				if (value.try_cast<Handle<Shader>>())
				{
					data.set(componentMeta, AssetManager::Instance->GetAsset<Shader>(node[memberName].as<UUID>()));
				}
			}
			else if (value.type() == entt::resolve<Handle<Material>>(ctx->GetMetaContext()))
			{
				if (value.try_cast<Handle<Material>>())
				{
					data.set(componentMeta, AssetManager::Instance->GetAsset<Material>(node[memberName].as<UUID>()));
				}
			}
			else if (value.type() == entt::resolve<Handle<Mesh>>(ctx->GetMetaContext()))
			{
				if (value.try_cast<Handle<Mesh>>())
				{
					data.set(componentMeta, AssetManager::Instance->GetAsset<Mesh>(node[memberName].as<UUID>()));
				}
			}
			else if (value.type() == entt::resolve<Handle<Texture>>(ctx->GetMetaContext()))
			{
				if (value.try_cast<Handle<Texture>>())
				{
					data.set(componentMeta, AssetManager::Instance->GetAsset<Texture>(node[memberName].as<UUID>()));
				}
			}
			else if (value.type() == entt::resolve<entt::entity>(ctx->GetMetaContext()))
			{
				if (value.try_cast<entt::entity>())
				{
					entt::entity entity = ctx->FindEntityByUUID(node[memberName].as<UUID>());
					data.set(componentMeta, entity);
				}
			}
			else if (value.type() == entt::resolve<Entity>(ctx->GetMetaContext()))
			{
				if (value.try_cast<Entity>())
				{
					Entity entity = ctx->FindEntityByUUID(node[memberName].as<UUID>());
					data.set(componentMeta, entity);
				}
			}
		}
		else
		{
			std::cerr << "Failed to retrieve member name!" << std::endl;
		}
	}
}
