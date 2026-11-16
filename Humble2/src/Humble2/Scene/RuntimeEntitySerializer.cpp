#include "RuntimeEntitySerializer.h"

#include "Asset/AssetManager.h"
#include "Serialization/Serializer.h"

namespace HBL2
{
	RuntimeEntitySerializer::RuntimeEntitySerializer(Scene* scene)
		: m_Scene(scene)
	{

	}
	
	RuntimeEntitySerializer::RuntimeEntitySerializer(Scene* scene, Entity entity)
		: m_Scene(scene), m_Entity(entity)
	{

	}

	void RuntimeEntitySerializer::Serialize(FileStreamWriter& stream)
	{
		if (m_Scene->HasComponent<Component::TerrainChunk>(m_Entity))
		{
			return;
		}

		stream.WriteRaw(m_Entity.Idx);

		uint64_t indexPos = stream.GetStreamPosition();
		int32_t entityComponentCount = 0;
		stream.WriteRaw(entityComponentCount);

		if (m_Scene->HasComponent<Component::Tag>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::Tag>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::Tag>::value);
			stream.WriteString(component.Name);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::ID>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::ID>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::ID>::value);
			stream.WriteRaw(component.Identifier);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::Transform>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::Transform>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::Transform>::value);
			stream.WriteRaw(component.Translation);
			stream.WriteRaw(component.Rotation);
			stream.WriteRaw(component.Scale);
			stream.WriteRaw(component.Static);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::PrefabInstance>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::PrefabInstance>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::PrefabInstance>::value);
			stream.WriteRaw(component.Id);
			stream.WriteRaw(component.Version);
			stream.WriteRaw(component.Override);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::PrefabEntity>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::PrefabEntity>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::PrefabEntity>::value);
			stream.WriteRaw(component.EntityId);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::Link>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::Link>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::Link>::value);
			stream.WriteRaw(component.Parent);
			stream.WriteSpan<UUID>(component.Children);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::Camera>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::Camera>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::Camera>::value);
			stream.WriteRaw(component.Enabled);
			stream.WriteRaw(component.Primary);
			stream.WriteRaw(component.Far);
			stream.WriteRaw(component.Near);
			stream.WriteRaw(component.Fov);
			stream.WriteRaw(component.AspectRatio);
			stream.WriteRaw(component.Exposure);
			stream.WriteRaw(component.Gamma);
			stream.WriteRaw(component.ZoomLevel);
			stream.WriteRaw((uint32_t)component.Type);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::Sprite>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::Sprite>(m_Entity);

			Asset* materialAsset = AssetManager::Instance->GetAssetMetadata(component.Material.Get());

			stream.WriteRaw(Reflect::TypeIndex<Component::Sprite>::value);
			stream.WriteRaw(component.Enabled);
			stream.WriteRaw((materialAsset != nullptr ? materialAsset->UUID : (UUID)0));

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::StaticMesh>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::StaticMesh>(m_Entity);

			Asset* materialAsset = AssetManager::Instance->GetAssetMetadata(component.Material.Get());
			Asset* meshAsset = AssetManager::Instance->GetAssetMetadata(component.Mesh.Get());

			stream.WriteRaw(Reflect::TypeIndex<Component::StaticMesh>::value);
			stream.WriteRaw(component.Enabled);
			stream.WriteRaw((materialAsset != nullptr ? materialAsset->UUID : (UUID)0));
			stream.WriteRaw((meshAsset != nullptr ? meshAsset->UUID : (UUID)0));
			stream.WriteRaw(component.MeshIndex);
			stream.WriteRaw(component.SubMeshIndex);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::SkinnedMesh>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::SkinnedMesh>(m_Entity);

			Asset* meshAsset = AssetManager::Instance->GetAssetMetadata(component.Mesh.Get());

			stream.WriteRaw(Reflect::TypeIndex<Component::SkinnedMesh>::value);
			stream.WriteRaw((meshAsset != nullptr ? meshAsset->UUID : (UUID)0));
			stream.WriteRaw(component.MeshIndex);
			stream.WriteRaw(component.SkeletonPoseUUID);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::SkinnedSubMesh>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::SkinnedSubMesh>(m_Entity);

			Asset* materialAsset = AssetManager::Instance->GetAssetMetadata(component.Material.Get());
			Asset* meshAsset = AssetManager::Instance->GetAssetMetadata(component.Mesh.Get());

			stream.WriteRaw(Reflect::TypeIndex<Component::SkinnedSubMesh>::value);
			stream.WriteRaw(component.Enabled);
			stream.WriteRaw((materialAsset != nullptr ? materialAsset->UUID : (UUID)0));
			stream.WriteRaw((meshAsset != nullptr ? meshAsset->UUID : (UUID)0));
			stream.WriteRaw(component.MeshIndex);
			stream.WriteRaw(component.SubMeshIndex);
			stream.WriteRaw(component.SkinnedMeshUUID);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::Animation>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::Animation>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::Animation>::value);
			stream.WriteRaw(component.Enabled);
			stream.WriteRaw(component.DefaultClip);
			stream.WriteRaw(component.Speed);
			stream.WriteRaw(component.TransitionDuration);
			stream.WriteRaw(component.EnableLooping);
			stream.WriteRaw(component.PlayAutomatically);
			stream.WriteRaw(component.ApplyRootMotion);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::SkeletonPose>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::SkeletonPose>(m_Entity);

			Asset* meshAsset = AssetManager::Instance->GetAssetMetadata(component.Mesh.Get());

			stream.WriteRaw(Reflect::TypeIndex<Component::SkeletonPose>::value);
			stream.WriteRaw((meshAsset != nullptr ? meshAsset->UUID : (UUID)0));

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::Light>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::Light>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::Light>::value);
			stream.WriteRaw(component.Enabled);
			stream.WriteRaw(component.CastsShadows);
			stream.WriteRaw(component.FollowMainCamera);
			stream.WriteRaw(component.Intensity);
			stream.WriteRaw(component.Color);
			stream.WriteRaw((uint32_t)component.Type);
			stream.WriteRaw(component.Distance);
			stream.WriteRaw(component.InnerCutOff);
			stream.WriteRaw(component.OuterCutOff);
			stream.WriteRaw(component.ConstantBias);
			stream.WriteRaw(component.SlopeBias);
			stream.WriteRaw(component.NormalOffsetScale);
			stream.WriteRaw(component.FieldOfView);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::SkyLight>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::SkyLight>(m_Entity);

			Asset* textureAsset = AssetManager::Instance->GetAssetMetadata(component.EquirectangularMap.Get());

			stream.WriteRaw(Reflect::TypeIndex<Component::SkyLight>::value);
			stream.WriteRaw(component.Enabled);
			stream.WriteRaw((textureAsset != nullptr ? textureAsset->UUID : (UUID)0));

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::AudioSource>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::AudioSource>(m_Entity);

			// TODO: Change sound handle from resource handle to asset handle.

			stream.WriteRaw(Reflect::TypeIndex<Component::AudioSource>::value);
			stream.WriteRaw(component.Volume);
			stream.WriteRaw(component.Pitch);
			stream.WriteRaw((uint32_t)(uint8_t)component.State);
			stream.WriteRaw((uint32_t)component.Flags);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::AudioListener>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::AudioListener>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::AudioListener>::value);
			stream.WriteRaw(component.Enabled);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::Rigidbody2D>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::Rigidbody2D>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::Rigidbody2D>::value);
			stream.WriteRaw(component.Enabled);
			stream.WriteRaw((int)component.Type);
			stream.WriteRaw(component.FixedRotation);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::BoxCollider2D>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::BoxCollider2D>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::BoxCollider2D>::value);
			stream.WriteRaw(component.Enabled);
			stream.WriteRaw(component.Trigger);
			stream.WriteRaw(component.Density);
			stream.WriteRaw(component.Friction);
			stream.WriteRaw(component.Restitution);
			stream.WriteRaw(component.Size);
			stream.WriteRaw(component.Offset);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::Rigidbody>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::Rigidbody>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::Rigidbody>::value);
			stream.WriteRaw(component.Enabled);
			stream.WriteRaw(component.Trigger);
			stream.WriteRaw((int)component.Type);
			stream.WriteRaw((int)component.MotionQuality);
			stream.WriteRaw(component.Mass);
			stream.WriteRaw(component.Friction);
			stream.WriteRaw(component.GravityFactor);
			stream.WriteRaw(component.Restitution);
			stream.WriteRaw(component.LinearDamping);
			stream.WriteRaw(component.AngularDamping);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::BoxCollider>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::BoxCollider>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::BoxCollider>::value);
			stream.WriteRaw(component.Enabled);
			stream.WriteRaw(component.Size);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::SphereCollider>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::SphereCollider>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::SphereCollider>::value);
			stream.WriteRaw(component.Enabled);
			stream.WriteRaw(component.Radius);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::CapsuleCollider>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::CapsuleCollider>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::CapsuleCollider>::value);
			stream.WriteRaw(component.Enabled);
			stream.WriteRaw(component.Height);
			stream.WriteRaw(component.Radius);

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::Terrain>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::Terrain>(m_Entity);

			Asset* materialAsset = AssetManager::Instance->GetAssetMetadata(component.Material.Get());

			stream.WriteRaw(Reflect::TypeIndex<Component::Terrain>::value);
			stream.WriteRaw((uint32_t)component.NormaliseMode);
			stream.WriteRaw(component.Seed);
			stream.WriteRaw(component.HeightMultiplier);
			stream.WriteRaw(component.Scale);
			stream.WriteRaw(component.NoiseScale);
			stream.WriteRaw(component.AddColliders);
			stream.WriteRaw(component.NumberOfChunks);
			stream.WriteRaw(component.UseFalloffMap);
			stream.WriteRaw(component.Regenerate);
			stream.WriteSpan<Component::Terrain::LodInfo>(component.DetailLevels);
			stream.WriteRaw((materialAsset != nullptr ? materialAsset->UUID : (UUID)0));

			entityComponentCount++;
		}

		if (m_Scene->HasComponent<Component::AnimationCurve>(m_Entity))
		{
			auto& component = m_Scene->GetComponent<Component::AnimationCurve>(m_Entity);

			stream.WriteRaw(Reflect::TypeIndex<Component::AnimationCurve>::value);
			stream.WriteRaw((uint32_t)component.Preset);
			stream.WriteSpan<Component::AnimationCurve::KeyFrame>(component.Keys);

			entityComponentCount++;
		}

		// TODO: Handle custom component serialization.

		// Save the end position to reset after patch.
		uint64_t endPos = stream.GetStreamPosition();

		// Reset cursor position.
		stream.SetStreamPosition(indexPos);

		// Write component count.
		stream.WriteRaw<int32_t>(entityComponentCount);

		// Set again the cursor at the end position.
		stream.SetStreamPosition(endPos);
	}

	bool RuntimeEntitySerializer::Deserialize(FileStreamReader& stream)
	{
		uint32_t entityID;
		stream.ReadRaw(entityID);

		int32_t entityComponentCount;
		stream.ReadRaw(entityComponentCount);

		// Retrieve tag component id.
		uint64_t tagComponentID;
		stream.ReadRaw(tagComponentID);

		// Retrieve component tag.
		StaticString<64> name;
		{
			stream.ReadString(name);
			entityComponentCount--;
		}

		// Retrieve id component id.
		uint64_t idComponentID;
		stream.ReadRaw(idComponentID);

		// Retrieve entity uuid.
		UUID uuid;
		{
			stream.ReadRaw(uuid);
			entityComponentCount--;
		}

		m_Entity = m_Scene->CreateEntityWithUUID(uuid, name);

		while (entityComponentCount > 0)
		{
			// Retrieve component id.
			uint64_t componentID;
			stream.ReadRaw(componentID);

			if (componentID == Reflect::TypeIndex<Component::Transform>::value)
			{
				auto& component = m_Scene->AddComponent<Component::Transform>(m_Entity);

				stream.ReadRaw(component.Translation);
				stream.ReadRaw(component.Rotation);
				stream.ReadRaw(component.Scale);
				stream.ReadRaw(component.Static);
			}
			else if (componentID == Reflect::TypeIndex<Component::PrefabInstance>::value)
			{
				auto& component = m_Scene->AddComponent<Component::PrefabInstance>(m_Entity);

				stream.ReadRaw(component.Id);
				stream.ReadRaw(component.Version);
				stream.ReadRaw(component.Override);
			}
			else if (componentID == Reflect::TypeIndex<Component::PrefabEntity>::value)
			{
				auto& component = m_Scene->AddComponent<Component::PrefabEntity>(m_Entity);

				stream.ReadRaw(component.EntityId);
			}
			else if (componentID == Reflect::TypeIndex<Component::Link>::value)
			{
				auto& component = m_Scene->AddComponent<Component::Link>(m_Entity);

				stream.ReadRaw(component.Parent);

				uint32_t childrenCount;
				stream.ReadRaw(childrenCount);
				component.Children.resize(childrenCount);
				stream.ReadSpan<UUID>(component.Children, childrenCount);
			}
			else if (componentID == Reflect::TypeIndex<Component::Camera>::value)
			{
				auto& component = m_Scene->AddComponent<Component::Camera>(m_Entity);

				stream.ReadRaw(component.Enabled);
				stream.ReadRaw(component.Primary);
				stream.ReadRaw(component.Far);
				stream.ReadRaw(component.Near);
				stream.ReadRaw(component.Fov);
				stream.ReadRaw(component.AspectRatio);
				stream.ReadRaw(component.Exposure);
				stream.ReadRaw(component.Gamma);
				stream.ReadRaw(component.ZoomLevel);
				uint32_t type;
				stream.ReadRaw(type);
				component.Type = (Component::Camera::EType)type;
			}
			else if (componentID == Reflect::TypeIndex<Component::Sprite>::value)
			{
				auto& component = m_Scene->AddComponent<Component::Sprite>(m_Entity);

				stream.ReadRaw(component.Enabled);

				UUID materialUUID;
				stream.ReadRaw(materialUUID);

				component.Material = AssetManager::Instance->GetHandleFromUUID(materialUUID);
				AssetManager::Instance->GetAsset<Material>(component.Material.Get());
			}
			else if (componentID == Reflect::TypeIndex<Component::StaticMesh>::value)
			{
				auto& component = m_Scene->AddComponent<Component::StaticMesh>(m_Entity);

				stream.ReadRaw(component.Enabled);

				UUID materialUUID;
				stream.ReadRaw(materialUUID);

				component.Material = AssetManager::Instance->GetHandleFromUUID(materialUUID);
				AssetManager::Instance->GetAsset<Material>(component.Material.Get());

				UUID meshUUID;
				stream.ReadRaw(meshUUID);

				component.Mesh = AssetManager::Instance->GetHandleFromUUID(meshUUID);
				AssetManager::Instance->GetAsset<Mesh>(component.Mesh.Get());

				stream.ReadRaw(component.MeshIndex);
				stream.ReadRaw(component.SubMeshIndex);
			}
			else if (componentID == Reflect::TypeIndex<Component::SkinnedMesh>::value)
			{
				auto& component = m_Scene->AddComponent<Component::SkinnedMesh>(m_Entity);

				UUID meshUUID;
				stream.ReadRaw(meshUUID);

				component.Mesh = AssetManager::Instance->GetHandleFromUUID(meshUUID);
				AssetManager::Instance->GetAsset<Mesh>(component.Mesh.Get());

				stream.ReadRaw(component.MeshIndex);
				stream.ReadRaw(component.SkeletonPoseUUID);
			}
			else if (componentID == Reflect::TypeIndex<Component::SkinnedSubMesh>::value)
			{
				auto& component = m_Scene->AddComponent<Component::SkinnedSubMesh>(m_Entity);

				stream.ReadRaw(component.Enabled);

				UUID materialUUID;
				stream.ReadRaw(materialUUID);

				component.Material = AssetManager::Instance->GetHandleFromUUID(materialUUID);
				AssetManager::Instance->GetAsset<Material>(component.Material.Get());

				UUID meshUUID;
				stream.ReadRaw(meshUUID);

				component.Mesh = AssetManager::Instance->GetHandleFromUUID(meshUUID);
				AssetManager::Instance->GetAsset<Mesh>(component.Mesh.Get());

				stream.ReadRaw(component.MeshIndex);
				stream.ReadRaw(component.SubMeshIndex);
				stream.ReadRaw(component.SkinnedMeshUUID);
			}
			else if (componentID == Reflect::TypeIndex<Component::Animation>::value)
			{
				auto& component = m_Scene->AddComponent<Component::Animation>(m_Entity);

				stream.ReadRaw(component.Enabled);
				stream.ReadRaw(component.DefaultClip);
				stream.ReadRaw(component.Speed);
				stream.ReadRaw(component.TransitionDuration);
				stream.ReadRaw(component.EnableLooping);
				stream.ReadRaw(component.PlayAutomatically);
				stream.ReadRaw(component.ApplyRootMotion);
			}
			else if (componentID == Reflect::TypeIndex<Component::SkeletonPose>::value)
			{
				auto& component = m_Scene->AddComponent<Component::SkeletonPose>(m_Entity);

				UUID meshUUID;
				stream.ReadRaw(meshUUID);

				component.Mesh = AssetManager::Instance->GetHandleFromUUID(meshUUID);
				AssetManager::Instance->GetAsset<Mesh>(component.Mesh.Get());
			}
			else if (componentID == Reflect::TypeIndex<Component::Light>::value)
			{
				auto& component = m_Scene->AddComponent<Component::Light>(m_Entity);

				stream.ReadRaw(component.Enabled);
				stream.ReadRaw(component.CastsShadows);
				stream.ReadRaw(component.FollowMainCamera);
				stream.ReadRaw(component.Intensity);
				stream.ReadRaw(component.Color);

				uint32_t type;
				stream.ReadRaw(type);
				component.Type = (Component::Light::EType)type;

				stream.ReadRaw(component.Distance);
				stream.ReadRaw(component.InnerCutOff);
				stream.ReadRaw(component.OuterCutOff);
				stream.ReadRaw(component.ConstantBias);
				stream.ReadRaw(component.SlopeBias);
				stream.ReadRaw(component.NormalOffsetScale);
				stream.ReadRaw(component.FieldOfView);
			}
			else if (componentID == Reflect::TypeIndex<Component::SkyLight>::value)
			{
				auto& component = m_Scene->AddComponent<Component::SkyLight>(m_Entity);

				stream.ReadRaw(component.Enabled);

				UUID textureUUID;
				stream.ReadRaw(textureUUID);

				component.EquirectangularMap = AssetManager::Instance->GetHandleFromUUID(textureUUID);
				AssetManager::Instance->GetAsset<Texture>(component.EquirectangularMap.Get());
			}
			else if (componentID == Reflect::TypeIndex<Component::AudioSource>::value)
			{
				auto& component = m_Scene->AddComponent<Component::AudioSource>(m_Entity);

				// TODO: Change sound handle from resource handle to asset handle.

				stream.ReadRaw(component.Volume);
				stream.ReadRaw(component.Pitch);

				uint32_t state;
				stream.ReadRaw(state);
				component.State = (Component::AudioSource::PlaybackState)state;

				uint32_t flags;
				stream.ReadRaw(flags);
				component.Flags = (uint8_t)flags;
			}
			else if (componentID == Reflect::TypeIndex<Component::AudioListener>::value)
			{
				auto& component = m_Scene->AddComponent<Component::AudioListener>(m_Entity);

				stream.ReadRaw(component.Enabled);
			}
			else if (componentID == Reflect::TypeIndex<Component::Rigidbody2D>::value)
			{
				auto& component = m_Scene->AddComponent<Component::Rigidbody2D>(m_Entity);

				stream.ReadRaw(component.Enabled);
				int type;
				stream.ReadRaw(type);
				component.Type = (Physics::BodyType)type;

				stream.ReadRaw(component.FixedRotation);
			}
			else if (componentID == Reflect::TypeIndex<Component::BoxCollider2D>::value)
			{
				auto& component = m_Scene->AddComponent<Component::BoxCollider2D>(m_Entity);

				stream.ReadRaw(component.Enabled);
				stream.ReadRaw(component.Trigger);
				stream.ReadRaw(component.Density);
				stream.ReadRaw(component.Friction);
				stream.ReadRaw(component.Restitution);
				stream.ReadRaw(component.Size);
				stream.ReadRaw(component.Offset);
			}
			else if (componentID == Reflect::TypeIndex<Component::Rigidbody>::value)
			{
				auto& component = m_Scene->AddComponent<Component::Rigidbody>(m_Entity);

				stream.ReadRaw(component.Enabled);
				stream.ReadRaw(component.Trigger);

				int type;
				stream.ReadRaw(type);
				component.Type = (Physics::BodyType)type;

				int motionQuality;
				stream.ReadRaw(motionQuality);
				component.MotionQuality = (Component::Rigidbody::EMotionQuality)motionQuality;

				stream.ReadRaw(component.Mass);
				stream.ReadRaw(component.Friction);
				stream.ReadRaw(component.GravityFactor);
				stream.ReadRaw(component.Restitution);
				stream.ReadRaw(component.LinearDamping);
				stream.ReadRaw(component.AngularDamping);
			}
			else if (componentID == Reflect::TypeIndex<Component::BoxCollider>::value)
			{
				auto& component = m_Scene->AddComponent<Component::BoxCollider>(m_Entity);

				stream.ReadRaw(component.Enabled);
				stream.ReadRaw(component.Size);
			}
			else if (componentID == Reflect::TypeIndex<Component::SphereCollider>::value)
			{
				auto& component = m_Scene->AddComponent<Component::SphereCollider>(m_Entity);

				stream.ReadRaw(component.Enabled);
				stream.ReadRaw(component.Radius);
			}
			else if (componentID == Reflect::TypeIndex<Component::CapsuleCollider>::value)
			{
				auto& component = m_Scene->AddComponent<Component::CapsuleCollider>(m_Entity);

				stream.ReadRaw(component.Enabled);
				stream.ReadRaw(component.Height);
				stream.ReadRaw(component.Radius);
			}
			else if (componentID == Reflect::TypeIndex<Component::Terrain>::value)
			{
				auto& component = m_Scene->AddComponent<Component::Terrain>(m_Entity);

				uint32_t normaliseMode;
				stream.ReadRaw(normaliseMode);
				component.NormaliseMode = (Component::Terrain::ENormaliseMode)normaliseMode;

				stream.ReadRaw(component.Seed);
				stream.ReadRaw(component.HeightMultiplier);
				stream.ReadRaw(component.Scale);
				stream.ReadRaw(component.NoiseScale);
				stream.ReadRaw(component.AddColliders);
				stream.ReadRaw(component.NumberOfChunks);
				stream.ReadRaw(component.UseFalloffMap);
				stream.ReadRaw(component.Regenerate);
				
				uint32_t detailLevelCount;
				stream.ReadRaw(detailLevelCount);
				component.DetailLevels.resize(detailLevelCount);
				stream.ReadSpan<Component::Terrain::LodInfo>(component.DetailLevels, detailLevelCount);

				UUID materialUUID;
				stream.ReadRaw(materialUUID);

				component.Material = AssetManager::Instance->GetHandleFromUUID(materialUUID);
				AssetManager::Instance->GetAsset<Material>(component.Material.Get());
			}
			else if (componentID == Reflect::TypeIndex<Component::AnimationCurve>::value)
			{
				auto& component = m_Scene->AddComponent<Component::AnimationCurve>(m_Entity);

				uint32_t preset;
				stream.ReadRaw(preset);
				component.Preset = (Component::AnimationCurve::CurvePreset)preset;
				
				uint32_t keyCount;
				stream.ReadRaw(keyCount);
				component.Keys.resize(keyCount);
				stream.ReadSpan<Component::AnimationCurve::KeyFrame>(component.Keys, keyCount);
			}

			// TODO: Handle custom component deserialization.

			entityComponentCount--;
		}

		return false;
	}
}
