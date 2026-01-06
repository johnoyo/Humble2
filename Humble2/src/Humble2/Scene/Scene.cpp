#include "Scene.h"

#include "ISystem.h"
#include "Utilities/NativeScriptUtilities.h"

#include "Project\Project.h"
#include "SceneSerializer.h"

#include "Systems\TransformSystem.h"
#include "Systems\LinkSystem.h"
#include "Systems\HierachySystem.h"
#include "Systems\CameraSystem.h"
#include "Systems\RenderingSystem.h"
#include "Systems\SoundSystem.h"
#include "Systems\Physics2dSystem.h"
#include "Systems\Physics3dSystem.h"
#include "Systems\TerrainSystem.h"
#include "Systems\AnimationCurveSystem.h"

namespace HBL2
{
    Scene::Scene(const SceneDescriptor&& desc) : m_Name(desc.name)
    {
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
        copy_component(Component::AnimationCurve{});
        copy_component(Component::Terrain{});

        // Do not copy the TerrainChunk component
        // copy_component(Component::TerrainChunk{});

        // Clone systems.
        //dst->RegisterSystem(new TransformSystem);
        //dst->RegisterSystem(new LinkSystem);
        dst->RegisterSystem(new HierachySystem);
        dst->RegisterSystem(new CameraSystem, SystemType::Runtime);
        dst->RegisterSystem(new TerrainSystem);
        dst->RegisterSystem(new RenderingSystem);
        dst->RegisterSystem(new SoundSystem, SystemType::Runtime);
        dst->RegisterSystem(new Physics2dSystem, SystemType::Runtime);
        dst->RegisterSystem(new Physics3dSystem, SystemType::Runtime);
        dst->RegisterSystem(new AnimationCurveSystem);

        // Register any user systems to new scene.
        for (ISystem* system : src->m_RuntimeSystems)
        {
            if (system->GetType() == SystemType::User)
            {
                NativeScriptUtilities::Get().RegisterSystem(system->Name, dst);
            }
        }

        // Clone user defined components.
        std::vector<std::string> userComponentNames;
        std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>> data;

        // Store all registered meta types of the source scene.
        for (auto meta_type : entt::resolve(src->GetMetaContext()))
        {
            std::string componentName = meta_type.second.info().name().data();
            componentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);
            userComponentNames.push_back(componentName);

            NativeScriptUtilities::Get().SerializeComponents(componentName, src, data, false);
        }

        // Copy the components to the new scene.
        for (const auto& userComponentName : userComponentNames)
        {
            NativeScriptUtilities::Get().DeserializeComponents(userComponentName, dst, data);
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

            const std::string& cleanedComponentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);
            NativeScriptUtilities::Get().ClearComponentStorage(cleanedComponentName, this);
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
                delete system;
            }
        }

        m_Systems.clear();
        m_CoreSystems.clear();
        m_RuntimeSystems.clear();
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

    Entity Scene::DuplicateEntity(Entity entity)
    {
        std::string name = GetComponent<Component::Tag>(entity).Name;
        Entity newEntity = CreateEntity(name + "(Clone)");
        auto& newLink = AddComponent<HBL2::Component::Link>(newEntity);

        // Helper lamda for component copying
        auto copy_component = [&](auto component_type)
        {
            using Component = decltype(component_type);

            if (HasComponent<Component>(entity))
            {
                auto& component = GetComponent<Component>(entity);

                if (typeid(Component) == typeid(HBL2::Component::Link))
                {
                    for (auto child : ((HBL2::Component::Link&)component).Children)
                    {
                        Entity childEntity = FindEntityByUUID(child);
                        Entity newChildEntity = DuplicateEntity(childEntity);

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
        copy_component(Component::AnimationCurve{});
        copy_component(Component::Terrain{});
        copy_component(Component::TerrainChunk{});

        // Copy user defined components.
        std::vector<std::string> userComponentNames;
        std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>> data;

        for (auto meta_type : entt::resolve(m_MetaContext))
        {
            std::string componentName = meta_type.second.info().name().data();
            componentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);

            if (NativeScriptUtilities::Get().HasComponent(componentName, this, entity))
            {
                auto componentMeta = NativeScriptUtilities::Get().GetComponent(componentName, this, entity);
                auto newComponentMeta = NativeScriptUtilities::Get().AddComponent(componentName, this, newEntity);
                newComponentMeta.assign(componentMeta);
            }
        }

        return newEntity;
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

        delete system;
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

    void Scene::operator=(const HBL2::Scene& other)
    {
        m_Name = other.m_Name;

        // NOTE: Here ^ we copy only the name of the scene.
        //       Its only used by the Pool class where the scene is empty
        //       and we only want to pass the name through.

        /*
        m_Systems = other.m_Systems;
        m_CoreSystems = other.m_CoreSystems;
        m_RuntimeSystems = other.m_RuntimeSystems;
        m_EntityMap = other.m_EntityMap;
        m_MetaContext = other.m_MetaContext;
        MainCamera = other.MainCamera;
        */
    }
}
