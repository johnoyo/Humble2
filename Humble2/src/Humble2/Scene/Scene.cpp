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
    Scene::Scene(const SceneDescriptor&& desc) : m_Name(desc.name)
    {
        // Minimal mode is only used in prefabs, in their sub scenes.
        if (desc.minimalMode)
        {
            m_Reservation = Allocator::Arena.Reserve("ScenePool", 1_MB);
            m_SceneArena.Initialize(&Allocator::Arena, 1_MB, m_Reservation);

            m_Systems = MakeDArray<ISystem*>(m_SceneArena, 1);
            m_CoreSystems = MakeDArray<ISystem*>(m_SceneArena, 1);
            m_RuntimeSystems = MakeDArray<ISystem*>(m_SceneArena, 1);
            m_EntityMap = MakeHMap<UUID, Entity>(m_SceneArena, 4096);

            return;
        }

        m_Reservation = Allocator::Arena.Reserve("ScenePool", 32_MB);
        m_SceneArena.Initialize(&Allocator::Arena, 16_MB, m_Reservation);

        m_Systems = MakeDArray<ISystem*>(m_SceneArena, 64);
        m_CoreSystems = MakeDArray<ISystem*>(m_SceneArena, 64);
        m_RuntimeSystems = MakeDArray<ISystem*>(m_SceneArena, 64);
        m_EntityMap = MakeHMap<UUID, Entity>(m_SceneArena, 32768);

        m_CmdBuffer = m_SceneArena.AllocConstruct<StructuralCommandBuffer>();
        m_CmdBuffer->Initialize(m_Reservation);
    }

    Scene* Scene::Copy(Scene* other)
    {
        HBL2_FUNC_PROFILE();

        Scene* newScene = new Scene({ .name = other->m_Name + "(Clone)"});

        Scene::Copy(other, newScene);

        return newScene;
    }

    void Scene::Copy(Scene* src, Scene* dst)
    {
        HBL2_FUNC_PROFILE();

        // Copy entites
        src->View<Component::ID>()
            .Each([&](Entity entity, Component::ID& id)
            {
                // Skip entities that are a terrain chunk.
                if (src->m_Registry.any_of<Component::TerrainChunk>(entity))
                {
                    return;
                }

                const auto& name = src->m_Registry.get<Component::Tag>(entity).Name;

                Entity newEntity = dst->m_Registry.create(entity);

                dst->m_Registry.emplace<Component::Tag>(newEntity).Name = name;
                dst->m_Registry.emplace<Component::ID>(newEntity).Identifier = id.Identifier;

                dst->m_EntityMap[id.Identifier] = newEntity;
            });

        // Helper lambda to copy components of a given type.
        auto copy_component = [&](auto component_type)
        {
            using Component = decltype(component_type);

            src->View<Component>()
                .Each([&](auto entity, const auto& component)
                {
                    // Skip entities that are a terrain chunk.
                    if (src->m_Registry.any_of<HBL2::Component::TerrainChunk>(entity))
                    {
                        return;
                    }

                    dst->m_Registry.emplace_or_replace<Component>(entity, component);
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
        std::vector<std::string> userComponentNames;
        std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>> data;

        // Store all registered meta types of the source scene.
        for (auto meta_type : entt::resolve(src->GetMetaContext()))
        {
            std::string componentName = meta_type.second.info().name().data();
            componentName = BuildEngine::Instance->CleanComponentNameO3(componentName);
            userComponentNames.push_back(componentName);

            BuildEngine::Instance->SerializeComponents(componentName, src, data, false);
        }

        // Copy the components to the new scene.
        for (const auto& userComponentName : userComponentNames)
        {
            BuildEngine::Instance->DeserializeComponents(userComponentName, dst, data);
        }

        // Set main camera.
        dst->MainCamera = src->MainCamera;
        dst->m_MetaContext = src->m_MetaContext;
    }

    void Scene::Clear()
    {
        // Clear user defined components.
        for (auto meta_type : entt::resolve(m_MetaContext))
        {
            const auto& alias = meta_type.second.info().name();

            if (alias.size() == 0 || alias.size() >= UINT32_MAX || alias.data() == nullptr)
            {
                continue;
            }

            const std::string& componentName = alias.data();

            const std::string& cleanedComponentName = BuildEngine::Instance->CleanComponentNameO3(componentName);
            BuildEngine::Instance->ClearComponentStorage(cleanedComponentName, this);
        }

        // Clear reflection system.
        entt::meta_reset(m_MetaContext);

        // Clear built in components.
        m_Registry.clear<Component::ID>();
        m_Registry.storage<Component::ID>().clear();
        m_Registry.compact<Component::ID>();

        m_Registry.clear<Component::Tag>();
        m_Registry.storage<Component::Tag>().clear();
        m_Registry.compact<Component::Tag>();

        m_Registry.clear<Component::EditorVisible>();
        m_Registry.storage<Component::EditorVisible>().clear();
        m_Registry.compact<Component::EditorVisible>();

        m_Registry.clear<Component::Transform>();
        m_Registry.storage<Component::Transform>().clear();
        m_Registry.compact<Component::Transform>();

        m_Registry.clear<Component::TransformEx>();
        m_Registry.storage<Component::TransformEx>().clear();
        m_Registry.compact<Component::TransformEx>();

        m_Registry.clear<Component::Link>();
        m_Registry.storage<Component::Link>().clear();
        m_Registry.compact<Component::Link>();

        m_Registry.clear<Component::Camera>();
        m_Registry.storage<Component::Camera>().clear();
        m_Registry.compact<Component::Camera>();

        m_Registry.clear<Component::Sprite>();
        m_Registry.storage<Component::Sprite>().clear();
        m_Registry.compact<Component::Sprite>();

        m_Registry.clear<Component::StaticMesh>();
        m_Registry.storage<Component::StaticMesh>().clear();
        m_Registry.compact<Component::StaticMesh>();

        m_Registry.clear<Component::Light>();
        m_Registry.storage<Component::Light>().clear();
        m_Registry.compact<Component::Light>();

        m_Registry.clear<Component::AudioSource>();
        m_Registry.storage<Component::AudioSource>().clear();
        m_Registry.compact<Component::AudioSource>();

        m_Registry.clear<Component::AudioListener>();
        m_Registry.storage<Component::AudioListener>().clear();
        m_Registry.compact<Component::AudioListener>();

        m_Registry.clear<Component::Rigidbody2D>();
        m_Registry.storage<Component::Rigidbody2D>().clear();
        m_Registry.compact<Component::Rigidbody2D>();

        m_Registry.clear<Component::BoxCollider2D>();
        m_Registry.storage<Component::BoxCollider2D>().clear();
        m_Registry.compact<Component::BoxCollider2D>();

        m_Registry.clear<Component::Rigidbody>();
        m_Registry.storage<Component::Rigidbody>().clear();
        m_Registry.compact<Component::Rigidbody>();

        m_Registry.clear<Component::BoxCollider>();
        m_Registry.storage<Component::BoxCollider>().clear();
        m_Registry.compact<Component::BoxCollider>();

        m_Registry.clear<Component::SphereCollider>();
        m_Registry.storage<Component::SphereCollider>().clear();
        m_Registry.compact<Component::SphereCollider>();

        m_Registry.clear<Component::CapsuleCollider>();
        m_Registry.storage<Component::CapsuleCollider>().clear();
        m_Registry.compact<Component::CapsuleCollider>();

        m_Registry.clear<Component::TerrainCollider>();
        m_Registry.storage<Component::TerrainCollider>().clear();
        m_Registry.compact<Component::TerrainCollider>();

        m_Registry.clear<Component::PrefabInstance>();
        m_Registry.storage<Component::PrefabInstance>().clear();
        m_Registry.compact<Component::PrefabInstance>();

        m_Registry.clear<Component::PrefabEntity>();
        m_Registry.storage<Component::PrefabEntity>().clear();
        m_Registry.compact<Component::PrefabEntity>();

        m_Registry.clear<Component::AnimationCurve>();
        m_Registry.storage<Component::AnimationCurve>().clear();
        m_Registry.compact<Component::AnimationCurve>();

        m_Registry.clear<Component::Terrain>();
        m_Registry.storage<Component::Terrain>().clear();
        m_Registry.compact<Component::Terrain>();

        m_Registry.clear<Component::TerrainChunk>();
        m_Registry.storage<Component::TerrainChunk>().clear();
        m_Registry.compact<Component::TerrainChunk>();

        // Destroy all entities.
        for (auto& [uuid, entity] : m_EntityMap)
        {
            m_Registry.destroy(entity);
        }

        m_EntityMap.clear();

        for (ISystem* system : m_Systems)
        {
            if (system != nullptr)
            {
                system->~ISystem();
            }
        }

        m_Systems.clear();
        m_CoreSystems.clear();
        m_RuntimeSystems.clear();

        if (m_CmdBuffer)
        {
            m_CmdBuffer->Clear();
            m_SceneArena.Destruct(m_CmdBuffer);
        }

        m_SceneArena.Destroy();
        m_Reservation = nullptr;
    }

    Entity Scene::CreateEntity()
    {
        return CreateEntityWithUUID(Random::UInt64());
    }

    Entity Scene::CreateEntity(const std::string& tag)
    {
        return CreateEntityWithUUID(Random::UInt64(), tag);
    }

    Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& tag)
    {
        Entity entity = m_Registry.create();

        m_Registry.emplace<Component::Tag>(entity).Name = tag;
        m_Registry.emplace<Component::ID>(entity).Identifier = uuid;
        m_Registry.emplace<Component::Transform>(entity);
        m_Registry.emplace<Component::TransformEx>(entity);
        m_Registry.emplace<Component::Link>(entity);

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
        std::string name = GetComponent<Component::Tag>(entity).Name;

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
        std::string name = otherScene->GetComponent<Component::Tag>(entity).Name;

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
        system->SetContext(this);
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
            HBL2_CORE_ERROR("Error while trying to destroy entity {0}. It is either invalid or does not have the built in required components.", (uint32_t)entity.Handle);
            return;
        }

        m_EntityMap.erase(id->Identifier);
        m_Registry.destroy(entity);
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
                    m_Registry.emplace_or_replace<Component>(newEntity, component);
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
        std::vector<std::string> userComponentNames;
        std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>> data;

        for (auto meta_type : entt::resolve(sourceEntityScene->m_MetaContext))
        {
            std::string componentName = meta_type.second.info().name().data();
            componentName = BuildEngine::Instance->CleanComponentNameO3(componentName);

            if (BuildEngine::Instance->HasComponent(componentName, sourceEntityScene, entity))
            {
                auto componentMeta = BuildEngine::Instance->GetComponent(componentName, sourceEntityScene, entity);
                auto newComponentMeta = BuildEngine::Instance->AddComponent(componentName, this, newEntity);
                newComponentMeta.assign(componentMeta);
            }
        }

        return newEntity;
    }

    Entity Scene::DuplicateEntityFromSceneAlt(Entity entity, Scene* sourceEntityScene, std::unordered_map<UUID, Entity>& preservedEntityIDs)
    {
        std::string name = sourceEntityScene->GetComponent<Component::Tag>(entity).Name;
        UUID uuid = Random::UInt64();

        Entity newEntity;

        if (auto* pe = sourceEntityScene->TryGetComponent<Component::PrefabEntity>(entity))
        {
            if (preservedEntityIDs.contains(pe->EntityId))
            {
                const Entity oldEntity = preservedEntityIDs.at(pe->EntityId);
                newEntity = m_Registry.create(oldEntity.Handle);
            }
            else
            {
                newEntity = m_Registry.create();
            }
        }
        else
        {
            newEntity = m_Registry.create();
        }

        m_Registry.emplace<Component::Tag>(newEntity).Name = name;
        m_Registry.emplace<Component::ID>(newEntity).Identifier = uuid;
        m_Registry.emplace<Component::Transform>(newEntity);
        m_Registry.emplace<Component::TransformEx>(newEntity);
        m_Registry.emplace<Component::Link>(newEntity);

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
                    m_Registry.emplace_or_replace<Component>(newEntity, component);
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
        std::vector<std::string> userComponentNames;
        std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>> data;

        for (auto meta_type : entt::resolve(sourceEntityScene->m_MetaContext))
        {
            std::string componentName = meta_type.second.info().name().data();
            componentName = BuildEngine::Instance->CleanComponentNameO3(componentName);

            if (BuildEngine::Instance->HasComponent(componentName, sourceEntityScene, entity))
            {
                auto componentMeta = BuildEngine::Instance->GetComponent(componentName, sourceEntityScene, entity);
                auto newComponentMeta = BuildEngine::Instance->AddComponent(componentName, this, newEntity);
                newComponentMeta.assign(componentMeta);
            }
        }

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

            if (m_Registry.valid(oldE.Handle))
            {
                m_Registry.destroy(oldE.Handle);
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
                m_EntityMap.erase(id.Identifier);

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
