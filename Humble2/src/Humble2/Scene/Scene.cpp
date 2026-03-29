#include "Scene.h"

#include "ISystem.h"

#include "Script\BuildEngine.h"
#include "Project\Project.h"
#include "SceneSerializer.h"

#include "Systems\HierachySystem.h"
#include "Systems\CameraSystem.h"
#include "Systems\RenderingSystem.h"
#include "Systems\SoundSystem.h"
#include "Systems\Physics2dSystem.h"
#include "Systems\Physics3dSystem.h"
#include "Systems\TerrainSystem.h"
#include "Systems\AnimationCurveSystem.h"

#include "StructuralCommandBuffer.h"

namespace HBL2
{
    Scene::Scene(const SceneDescriptor&& desc)
    {
        m_Descriptor = desc;

        m_Registry.Initialize(desc.maxEntities, desc.maxComponents);

        // Memory requirements for scene arena.
        uint64_t totalBytes = 0;

        totalBytes += desc.maxSystems * sizeof(ISystem*);
        totalBytes += desc.maxSystems * sizeof(ISystem*);
        totalBytes += desc.maxSystems * sizeof(ISystem*);
        totalBytes += desc.maxEntities * sizeof(std::pair<UUID, Entity>) * 2;
        totalBytes += desc.maxEntities * sizeof(uint64_t) * 2;
        totalBytes += sizeof(StructuralCommandBuffer);
        totalBytes += 100_KB;

        uint64_t sceneArenaBytes = totalBytes;

        // Calculate and add StructuralCommandBuffer memory requirements.
        uint32_t mainStructuralCommandBufferArenaByteSize = 0;
        if (desc.useStructuralCommandBuffer)
        {
            uint32_t workerThreadCount = JobSystem::Get().GetThreadCount();

            mainStructuralCommandBufferArenaByteSize = (uint32_t)Allocator::CalculateInterleavedByteSize<StructuralCommandBuffer::ChunkCommands, Arena>(workerThreadCount);
            totalBytes += mainStructuralCommandBufferArenaByteSize;

            totalBytes += (desc.maxStructuralCommandsPerFramePerThread * (sizeof(StructuralCommandBuffer::Command) + 128_B)) * workerThreadCount;
            totalBytes += 100_KB;
        }

        // Allocate memory for scene.
        m_Reservation = Allocator::Arena.Reserve("ScenePool", totalBytes);
        m_SceneArena.Initialize(&Allocator::Arena, sceneArenaBytes, m_Reservation);

        // Create scene data structures from arena.
        m_Systems = MakeDArray<ISystem*>(m_SceneArena, desc.maxSystems);
        m_CoreSystems = MakeDArray<ISystem*>(m_SceneArena, desc.maxSystems);
        m_RuntimeSystems = MakeDArray<ISystem*>(m_SceneArena, desc.maxSystems);

        m_EntityMap = HashMap<UUID, Entity>(&m_SceneArena, desc.maxEntities * 2);

        // Create the StructuralCommandBuffer if is requested.
        // (This is optional since for example in prefabs, which have subscenes embeded in them, we dont need it)
        if (desc.useStructuralCommandBuffer)
        {
            m_CmdBuffer = m_SceneArena.AllocConstruct<StructuralCommandBuffer>();
            m_CmdBuffer->Initialize(m_Reservation, mainStructuralCommandBufferArenaByteSize, desc.maxStructuralCommandsPerFramePerThread);
        }
    }

    Scene* Scene::Copy(Scene* other)
    {
        HBL2_FUNC_PROFILE();

        auto newSceneHandle = ResourceManager::Instance->CreateScene({
            .name = other->m_Descriptor.name + "(Clone)",
            .maxEntities = other->m_Descriptor.maxEntities,
            .maxComponents = other->m_Descriptor.maxComponents,
            .maxSystems = other->m_Descriptor.maxSystems,
            .maxStructuralCommandsPerFramePerThread = other->m_Descriptor.maxStructuralCommandsPerFramePerThread,
            .maxJobsPerSystem = other->m_Descriptor.maxJobsPerSystem,
            .useStructuralCommandBuffer = other->m_Descriptor.useStructuralCommandBuffer,
        });

        Scene* newScene = ResourceManager::Instance->GetScene(newSceneHandle);

        Scene::Copy(other, newScene);

        return newScene;
    }

    void Scene::Copy(Scene* src, Scene* dst)
    {
        HBL2_FUNC_PROFILE();

        // Copy entites
        src->Filter<Component::ID>()
            .ForEach([&](Entity entity, Component::ID& id)
            {
                // Skip entities that are a terrain chunk.
                if (src->m_Registry.HasComponent<Component::TerrainChunk>(entity))
                {
                    return;
                }

                const auto& name = src->m_Registry.GetComponent<Component::Tag>(entity).Name;

                Entity newEntity = dst->m_Registry.CreateEntity(entity);

                dst->m_Registry.AddComponent<Component::Tag>(newEntity).Name = name;
                dst->m_Registry.AddComponent<Component::ID>(newEntity).Identifier = id.Identifier;

                dst->m_EntityMap[id.Identifier] = newEntity;
            });

        // Helper lambda to copy components of a given type.
        auto copy_component = [&](auto component_type)
        {
            using Component = decltype(component_type);

            src->Filter<Component>()
                .ForEach([&](Entity entity, Component& component)
                {
                    // Skip entities that are a terrain chunk.
                    if (src->m_Registry.HasComponent<HBL2::Component::TerrainChunk>(entity))
                    {
                        return;
                    }

                    dst->m_Registry.AddOrReplaceComponent<Component>(entity, std::forward<Component>(component));
                });
        };

        // Copy components.
        copy_component(Component::Transform{});
        copy_component(Component::TransformEx{});
        copy_component(Component::Link{});
        copy_component(Component::Camera{});
        copy_component(Component::EditorVisible{});
        copy_component(Component::Sprite{});
        copy_component(Component::StaticMesh{});
        copy_component(Component::Light{});
        copy_component(Component::SkyLight{});
        copy_component(Component::AudioListener{});
        copy_component(Component::AudioSource{});
        copy_component(Component::Rigidbody2D{});
        copy_component(Component::BoxCollider2D{});
        copy_component(Component::Rigidbody{});
        copy_component(Component::BoxCollider{});
        copy_component(Component::SphereCollider{});
        copy_component(Component::CapsuleCollider{});
        copy_component(Component::TerrainCollider{});
        copy_component(Component::PrefabInstance{});
        copy_component(Component::PrefabEntity{});
        copy_component(Component::AnimationCurve{});
        copy_component(Component::Terrain{});
        // Do not copy the TerrainChunk component

        // Clone systems.
        dst->RegisterSystem<HierachySystem>();
        dst->RegisterSystem<CameraSystem>(SystemType::Runtime);
        dst->RegisterSystem<TerrainSystem>();
        dst->RegisterSystem<RenderingSystem>();
        dst->RegisterSystem<SoundSystem>(SystemType::Runtime);
        dst->RegisterSystem<Physics2dSystem>(SystemType::Runtime);
        dst->RegisterSystem<Physics3dSystem>(SystemType::Runtime);
        dst->RegisterSystem<AnimationCurveSystem>();

        // Register any user systems to new scene.
        for (ISystem* system : src->m_RuntimeSystems)
        {
            if (system->GetType() == SystemType::User)
            {
                BuildEngine::Instance->RegisterSystem(system->Name, dst);
            }
        }

        // Clone user defined components.
        Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
        {
            if (entry.cloneToRegistry)
            {
                entry.cloneToRegistry(&src->m_Registry, &dst->m_Registry);
            }
        });

        // Set main camera.
        dst->MainCamera = src->MainCamera;
    }

    void Scene::Clear()
    {
        // Clear registry (clears components and entities).
        m_Registry.Clear();

        // Clear entity to uuid mapping.
        m_EntityMap.clear();

        // Destruct systems.
        for (ISystem* system : m_Systems)
        {
            if (system != nullptr)
            {
                system->~ISystem();
            }
        }

        // Clear systems arrays.
        m_Systems.clear();
        m_CoreSystems.clear();
        m_RuntimeSystems.clear();

        // Clear and destruct structural command buffer.
        if (m_CmdBuffer)
        {
            m_CmdBuffer->Clear();
            m_SceneArena.Destruct(m_CmdBuffer);
        }

        // Finally, destroy scene arena.
        m_SceneArena.Destroy();
        m_Reservation = nullptr;
    }

    Entity Scene::CreateEntity()
    {
        return CreateEntityWithUUID(Random::UInt64());
    }

    Entity Scene::CreateEntity(const StaticString<64>& tag)
    {
        return CreateEntityWithUUID(Random::UInt64(), tag);
    }

    Entity Scene::CreateEntityWithUUID(UUID uuid, const StaticString<64>& tag)
    {
        Entity entity = m_Registry.CreateEntity();

        m_Registry.AddComponent<Component::Tag>(entity).Name = tag;
        m_Registry.AddComponent<Component::ID>(entity).Identifier = uuid;
        m_Registry.AddComponent<Component::Transform>(entity);
        m_Registry.AddComponent<Component::TransformEx>(entity);
        m_Registry.AddComponent<Component::Link>(entity);

        m_EntityMap[uuid] = entity;

        return entity;
    }

    Entity Scene::FindEntityByUUID(UUID uuid)
    {
        auto it = m_EntityMap.find(uuid);

        if (it != m_EntityMap.end())
        {
            return it->second;
        }

        return Entity::Null;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        InternalDestroyEntity(entity, true);
    }

    Entity Scene::DuplicateEntity(Entity entity, EntityDuplicationNaming namingConvention)
    {
        const auto& name = GetComponent<Component::Tag>(entity).Name;

        switch (namingConvention)
        {
        case HBL2::EntityDuplicationNaming::APPEND_CLONE_RECURSIVE:
            {
                Entity newEntity = CreateEntity(name + "(Clone)");
                return InternalDuplicateEntity(entity, this, newEntity, true);
            }
        case HBL2::EntityDuplicationNaming::APPEND_CLONE_TO_BASE_ONLY:
            {
                Entity newEntity = CreateEntity(name + "(Clone)");
                return InternalDuplicateEntity(entity, this, newEntity, false);
            }
        case HBL2::EntityDuplicationNaming::DONT_APPEND_CLONE:
            {
                Entity newEntity = CreateEntity(name);
                return InternalDuplicateEntity(entity, this, newEntity, false);
            }
        }

        return Entity::Null;
    }

    Entity Scene::DuplicateEntityFromScene(Entity entity, Scene* otherScene, EntityDuplicationNaming namingConvention)
    {
        const auto& name = otherScene->GetComponent<Component::Tag>(entity).Name;

        switch (namingConvention)
        {
        case HBL2::EntityDuplicationNaming::APPEND_CLONE_RECURSIVE:
            {
                Entity newEntity = CreateEntity(name + "(Clone)");
                return InternalDuplicateEntity(entity, otherScene, newEntity, true);
            }
        case HBL2::EntityDuplicationNaming::APPEND_CLONE_TO_BASE_ONLY:
            {
                Entity newEntity = CreateEntity(name + "(Clone)");
                return InternalDuplicateEntity(entity, otherScene, newEntity, false);
            }
        case HBL2::EntityDuplicationNaming::DONT_APPEND_CLONE:
            {
                Entity newEntity = CreateEntity(name);
                return InternalDuplicateEntity(entity, otherScene, newEntity, false);
            }
        }

        return Entity::Null;
    }

    void Scene::DeregisterSystem(const std::string& systemName)
    {
        ISystem* systemToBeDeleted = nullptr;

        for (ISystem* system : m_Systems)
        {
            if (system->Name == systemName)
            {
                systemToBeDeleted = system;
                break;
            }
        }

        DeregisterSystem(systemToBeDeleted);
    }

    void Scene::DeregisterSystem(ISystem* system)
    {
        if (system == nullptr)
        {
            return;
        }

        {
            // Erase from systems vector
            auto it = std::find(m_Systems.begin(), m_Systems.end(), system);

            if (it != m_Systems.end())
            {
                m_Systems.erase(it);
            }
        }

        {
            // Erase from core systems vector
            auto it = std::find(m_CoreSystems.begin(), m_CoreSystems.end(), system);

            if (it != m_CoreSystems.end())
            {
                m_CoreSystems.erase(it);
            }
        }

        {
            // Erase from runtime systems vector
            auto it = std::find(m_RuntimeSystems.begin(), m_RuntimeSystems.end(), system);

            if (it != m_RuntimeSystems.end())
            {
                m_RuntimeSystems.erase(it);
            }
        }

        system->~ISystem();
    }

    void Scene::RegisterSystem(ISystem* system, SystemType type)
    {
        system->SetType(type);
        system->SetContext(this, m_Descriptor.maxComponents);
        m_Systems.push_back(system);

        switch (type)
        {
        case HBL2::SystemType::Core:
            m_CoreSystems.push_back(system);
            break;
        case HBL2::SystemType::Runtime:
            m_RuntimeSystems.push_back(system);
            break;
        case HBL2::SystemType::User:
            m_RuntimeSystems.push_back(system);
            break;
        }
    }

    void Scene::InternalDestroyEntity(Entity entity, bool isRootCall)
    {
        auto* link = TryGetComponent<Component::Link>(entity);

        if (link)
        {
            // Save parent UUID, since for some reason when deleting a child which has other children,
            // when returning from the recursive deletion of those children the starting entity which was right clicked 
            // to be deleted has changed and has no children and the parent UUID of the child entity that was deleted.
            // I have no idea why!
            UUID parentId = link->Parent;

            for (auto child : link->Children)
            {
                Entity childEntity = FindEntityByUUID(child);
                InternalDestroyEntity(childEntity, false);
            }

            // Remove this entity from its parent children list, if it has a parent.
            if (parentId != 0 && isRootCall)
            {
                Entity parentEntity = FindEntityByUUID(parentId);
                auto* parentLink = TryGetComponent<Component::Link>(parentEntity);

                if (parentLink)
                {
                    UUID uuid = GetComponent<Component::ID>(entity).Identifier;
                    auto childrenIterator = std::find(parentLink->Children.begin(), parentLink->Children.end(), uuid);

                    if (childrenIterator != parentLink->Children.end())
                    {
                        parentLink->Children.erase(childrenIterator);
                    }
                }
            }
        }

        Component::ID* id = TryGetComponent<Component::ID>(entity);

        if (id == nullptr)
        {
            HBL2_CORE_ERROR("Error while trying to destroy entity {0}. It is either invalid or does not have the built in required components.", (uint32_t)entity.Idx);
            return;
        }

        m_EntityMap.remove(id->Identifier);
        m_Registry.DestroyEntity(entity);
    }

    Entity Scene::InternalDuplicateEntity(Entity entity, Scene* sourceEntityScene, Entity newEntity, bool appendCloneToName)
    {
        auto& newLink = GetComponent<HBL2::Component::Link>(newEntity);

        // Helper lamda for component copying
        auto copy_component = [&](auto component_type)
        {
            using Component = decltype(component_type);

            if (sourceEntityScene->HasComponent<Component>(entity))
            {
                auto& component = sourceEntityScene->GetComponent<Component>(entity);

                if (typeid(Component) == typeid(HBL2::Component::Link))
                {
                    for (auto child : ((HBL2::Component::Link&)component).Children)
                    {
                        Entity childEntity = sourceEntityScene->FindEntityByUUID(child);
                        Entity newChildEntity = DuplicateEntityFromScene(childEntity, sourceEntityScene, appendCloneToName ? EntityDuplicationNaming::APPEND_CLONE_RECURSIVE : EntityDuplicationNaming::DONT_APPEND_CLONE);

                        // Add the base entity as the parent of this
                        HBL2::Component::Link& newChildLink = GetComponent<HBL2::Component::Link>(newChildEntity);
                        newChildLink.Parent = GetComponent<HBL2::Component::ID>(newEntity).Identifier;
                        newChildLink.PrevParent = newChildLink.Parent;

                        // Add the new child entity to the new base entity
                        newLink.Children.push_back(GetComponent<HBL2::Component::ID>(newChildEntity).Identifier);
                    }
                }
                else
                {
                    m_Registry.AddOrReplaceComponent<Component>(newEntity, std::forward<Component>(component));
                }

            }
        };

        // Copy built in components
        copy_component(Component::Transform{});
        copy_component(Component::TransformEx{});
        copy_component(Component::Link{});
        copy_component(Component::Camera{});
        copy_component(Component::EditorVisible{});
        copy_component(Component::Sprite{});
        copy_component(Component::StaticMesh{});
        copy_component(Component::Light{});
        copy_component(Component::SkyLight{});
        copy_component(Component::AudioListener{});
        copy_component(Component::AudioSource{});
        copy_component(Component::Rigidbody2D{});
        copy_component(Component::BoxCollider2D{});
        copy_component(Component::Rigidbody{});
        copy_component(Component::BoxCollider{});
        copy_component(Component::SphereCollider{});
        copy_component(Component::CapsuleCollider{});
        copy_component(Component::TerrainCollider{});
        copy_component(Component::PrefabInstance{});
        copy_component(Component::PrefabEntity{});
        copy_component(Component::AnimationCurve{});
        copy_component(Component::Terrain{});
        copy_component(Component::TerrainChunk{});

        // Copy user defined components.
        Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
        {
            if (entry.hasInRegistry(&sourceEntityScene->m_Registry, entity))
            {
                auto componentMeta = entry.getFromRegistry(&sourceEntityScene->m_Registry, entity);
                auto newComponentMeta = entry.addToRegistry(&this->m_Registry, newEntity);
                entry.copy(newComponentMeta.Ptr, componentMeta.Ptr);
            }
        });

        return newEntity;
    }

    Entity Scene::DuplicateEntityFromSceneAlt(Entity entity, Scene* sourceEntityScene, std::unordered_map<UUID, Entity>& preservedEntityIDs)
    {
        const auto& name = sourceEntityScene->GetComponent<Component::Tag>(entity).Name;
        UUID uuid = Random::UInt64();

        Entity newEntity;

        if (auto* pe = sourceEntityScene->TryGetComponent<Component::PrefabEntity>(entity))
        {
            if (preservedEntityIDs.contains(pe->EntityId))
            {
                const Entity oldEntity = preservedEntityIDs.at(pe->EntityId);
                newEntity = m_Registry.CreateEntity(oldEntity);
            }
            else
            {
                newEntity = m_Registry.CreateEntity();
            }
        }
        else
        {
            newEntity = m_Registry.CreateEntity();
        }

        m_Registry.AddComponent<Component::Tag>(newEntity).Name = name;
        m_Registry.AddComponent<Component::ID>(newEntity).Identifier = uuid;
        m_Registry.AddComponent<Component::Transform>(newEntity);
        m_Registry.AddComponent<Component::TransformEx>(newEntity);
        m_Registry.AddComponent<Component::Link>(newEntity);

        m_EntityMap[uuid] = newEntity;

        return InternalDuplicateEntityFromSceneAlt(entity, sourceEntityScene, newEntity, preservedEntityIDs);
    }

    Entity Scene::InternalDuplicateEntityFromSceneAlt(Entity entity, Scene* sourceEntityScene, Entity newEntity, std::unordered_map<UUID, Entity>& preservedEntityIDs)
    {
        auto& newLink = GetComponent<HBL2::Component::Link>(newEntity);

        // Helper lamda for component copying
        auto copy_component = [&](auto component_type)
        {
            using Component = decltype(component_type);

            if (sourceEntityScene->HasComponent<Component>(entity))
            {
                auto& component = sourceEntityScene->GetComponent<Component>(entity);

                if (typeid(Component) == typeid(HBL2::Component::Link))
                {
                    for (auto child : ((HBL2::Component::Link&)component).Children)
                    {
                        Entity childEntity = sourceEntityScene->FindEntityByUUID(child);
                        Entity newChildEntity = DuplicateEntityFromSceneAlt(childEntity, sourceEntityScene, preservedEntityIDs);

                        // Add the base entity as the parent of this
                        HBL2::Component::Link& newChildLink = GetComponent<HBL2::Component::Link>(newChildEntity);
                        newChildLink.Parent = GetComponent<HBL2::Component::ID>(newEntity).Identifier;
                        newChildLink.PrevParent = newChildLink.Parent;

                        // Add the new child entity to the new base entity
                        newLink.Children.push_back(GetComponent<HBL2::Component::ID>(newChildEntity).Identifier);
                    }
                }
                else
                {
                    m_Registry.AddOrReplaceComponent<Component>(newEntity, std::forward<Component>(component));
                }

            }
        };

        // Copy built in components
        copy_component(Component::Transform{});
        copy_component(Component::TransformEx{});
        copy_component(Component::Link{});
        copy_component(Component::Camera{});
        copy_component(Component::EditorVisible{});
        copy_component(Component::Sprite{});
        copy_component(Component::StaticMesh{});
        copy_component(Component::Light{});
        copy_component(Component::SkyLight{});
        copy_component(Component::AudioListener{});
        copy_component(Component::AudioSource{});
        copy_component(Component::Rigidbody2D{});
        copy_component(Component::BoxCollider2D{});
        copy_component(Component::Rigidbody{});
        copy_component(Component::BoxCollider{});
        copy_component(Component::SphereCollider{});
        copy_component(Component::CapsuleCollider{});
        copy_component(Component::TerrainCollider{});
        copy_component(Component::PrefabInstance{});
        copy_component(Component::PrefabEntity{});
        copy_component(Component::AnimationCurve{});
        copy_component(Component::Terrain{});
        copy_component(Component::TerrainChunk{});

        // Copy user defined components.
        Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
        {
            if (entry.hasInRegistry(&sourceEntityScene->m_Registry, entity))
            {
                auto componentMeta = entry.getFromRegistry(&sourceEntityScene->m_Registry, entity);
                auto newComponentMeta = entry.addToRegistry(&this->m_Registry, newEntity);
                entry.copy(newComponentMeta.Ptr, componentMeta.Ptr);
            }
        });

        return newEntity;
    }

    Entity Scene::DuplicateEntityWhilePreservingUUIDsFromEntityAndDestroy(Entity prefabSourceEntity, Scene* prefabSourceScene, Entity entityToPreserveFrom)
    {
        // Store the entities of the current instantiated prefab entity.
        // We need to delete them in the end but we wont have the UUID to Entity mapping so we store them beforehand.
        std::function<void(Entity, std::vector<Entity>&)> collect = [&](Entity e, std::vector<Entity>& originals)
        {
            if (e == Entity::Null)
            {
                return;
            }

            originals.push_back(e);

            auto* link = TryGetComponent<Component::Link>(e);
            if (!link)
            {
                return;
            }

            for (UUID childUUID : link->Children)
            {
                Entity child = FindEntityByUUID(childUUID);
                if (child != Entity::Null)
                {
                    collect(child, originals);
                }
            }
        };

        std::vector<Entity> originals1;
        originals1.reserve(16);
        collect(entityToPreserveFrom, originals1);

        // Create a PrefabEntity to entity UUID mapping, so that after the duplication
        // we can preserve their UUIDs by checking this component in order to find the old matching UUID.
        std::unordered_map<UUID, UUID> preservedUUIDs;
        std::unordered_map<UUID, Entity> preservedEntityIDs;

        auto gatherPreservedUUIDs = [&](auto&& self, Entity e) -> void
        {
            if (e == Entity::Null)
            {
                return;
            }

            if (HasComponent<Component::PrefabEntity>(e))
            {
                const auto& pe = GetComponent<Component::PrefabEntity>(e);
                const auto& id = GetComponent<Component::ID>(e);
                preservedUUIDs[pe.EntityId] = id.Identifier;
                preservedEntityIDs[pe.EntityId] = e;
            }

            const auto& link = GetComponent<Component::Link>(e);
            for (UUID childUUID : link.Children)
            {
                Entity child = FindEntityByUUID(childUUID);
                if (child != Entity::Null)
                {
                    self(self, child);
                }
            }
        };
        
        gatherPreservedUUIDs(gatherPreservedUUIDs, entityToPreserveFrom);

        // Destroy the entity that was instantiated in the scene before from the cached entities we stored above.
        for (auto it = originals1.rbegin(); it != originals1.rend(); ++it)
        {
            Entity oldE = *it;
            if (oldE == Entity::Null)
            {
                continue;
            }

            if (m_Registry.IsValid(oldE))
            {
                m_Registry.DestroyEntity(oldE);
            }
        }

        // Dupilcate the entity from the prefab source entity.
        Entity clone = DuplicateEntityFromSceneAlt(prefabSourceEntity, prefabSourceScene, preservedEntityIDs);

        auto updateUUIDsFromPreserved = [&](auto&& self, Entity e, Entity parent) -> void
        {
            if (e == Entity::Null)
            {
                return;
            }

            // First restore the UUID in the ID component and update the entity map.
            if (HasComponent<Component::PrefabEntity>(e))
            {
                const auto& pe = GetComponent<Component::PrefabEntity>(e);
                auto& id = GetComponent<Component::ID>(e);
                m_EntityMap.remove(id.Identifier);

                if (preservedUUIDs.contains(pe.EntityId))
                {
                    id.Identifier = preservedUUIDs[pe.EntityId];
                }

                m_EntityMap[id.Identifier] = e;
            }

            auto& link = GetComponent<Component::Link>(e);

            // Then, update the parent UUID in the Link component if applicable.
            if (parent != Entity::Null)
            {
                auto& parentID = GetComponent<Component::ID>(parent);
                link.Parent = parentID.Identifier;
                link.PrevParent = parentID.Identifier;
            }

            // Do the update resursively for each child, while also updating the UUIDs in the child array.
            for (UUID& childUUID : link.Children)
            {
                Entity child = FindEntityByUUID(childUUID);
                if (child != Entity::Null)
                {
                    self(self, child, e);
                }

                childUUID = GetComponent<Component::ID>(child).Identifier;
            }
        };

        updateUUIDsFromPreserved(updateUUIDsFromPreserved, clone, Entity::Null);        

        return clone;
    }
    
    void Scene::PlaybackStructuralChanges()
    {
        m_CmdBuffer->Playback(this);
    }

    void Scene::AdvanceEpoch()
    {
        ++m_Epoch;
    }
}
