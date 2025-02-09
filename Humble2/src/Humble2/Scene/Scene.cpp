#include "Scene.h"

#include "Utilities/NativeScriptUtilities.h"

#include "Project\Project.h"
#include "SceneSerializer.h"

#include "Systems\TransformSystem.h"
#include "Systems\LinkSystem.h"
#include "Systems\CameraSystem.h"
#include "Systems\StaticMeshRenderingSystem.h"
#include "Systems\SpriteRenderingSystem.h"
#include "Systems\CompositeRenderingSystem.h"

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
        src->m_Registry
            .view<Component::ID>()
            .each([&](entt::entity entity, Component::ID& id)
            {
                const auto& name = src->m_Registry.get<Component::Tag>(entity).Name;

                entt::entity newEntity = dst->m_Registry.create(entity);

                dst->m_Registry.emplace<Component::Tag>(newEntity).Name = name;
                dst->m_Registry.emplace<Component::ID>(newEntity).Identifier = id.Identifier;

                dst->m_EntityMap[id.Identifier] = newEntity;
            });

        // Helper lambda to copy components of a given type.
        auto copy_component = [&](auto component_type)
        {
            using Component = decltype(component_type);

            src->m_Registry
                .view<Component>()
                .each([&](auto entity, const auto& component)
                {
                    dst->m_Registry.emplace_or_replace<Component>(entity, component);
                });
        };

        // Copy components.
        copy_component(Component::Transform{});
        copy_component(Component::Link{});
        copy_component(Component::Camera{});
        copy_component(Component::EditorVisible{});
        copy_component(Component::Sprite_New{});
        copy_component(Component::StaticMesh_New{});
        copy_component(Component::Light{});

        // Clone systems.
        dst->RegisterSystem(new TransformSystem);
        dst->RegisterSystem(new LinkSystem);
        dst->RegisterSystem(new CameraSystem, SystemType::Runtime);
        dst->RegisterSystem(new StaticMeshRenderingSystem);
        dst->RegisterSystem(new SpriteRenderingSystem);
        dst->RegisterSystem(new CompositeRenderingSystem);

        // Register any user systems to new scene.
        for (ISystem* system : src->m_RuntimeSystems)
        {
            if (system->GetType() == SystemType::User)
            {
                NativeScriptUtilities::Get().RegisterSystem(system->Name, dst); // NOTE: If no systems present now the new dll does not get loaded.
            }
        }

        // Clone user defined components.
        std::vector<std::string> userComponentNames;
        std::unordered_map<std::string, std::unordered_map<entt::entity, std::vector<std::byte>>> data;

        // Store all registered meta types of the source scene.
        for (auto meta_type : entt::resolve(src->GetMetaContext()))
        {
            std::string componentName = meta_type.second.info().name().data();
            componentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);
            userComponentNames.push_back(componentName);

            NativeScriptUtilities::Get().SerializeComponents(componentName, src, data, false);
        }

        // Register the components to the new scene.
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
            const std::string& componentName = meta_type.second.info().name().data();

            const std::string& cleanedComponentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);
            NativeScriptUtilities::Get().ClearComponentStorage(cleanedComponentName, this);
        }

        // Reset reflection system.
        entt::meta_reset(m_MetaContext);

        // Clear built in components.
        m_Registry.clear<HBL2::Component::ID>();
        m_Registry.storage<HBL2::Component::ID>().clear();
        m_Registry.compact<HBL2::Component::ID>();

        m_Registry.clear<HBL2::Component::Tag>();
        m_Registry.storage<HBL2::Component::Tag>().clear();
        m_Registry.compact<HBL2::Component::Tag>();

        m_Registry.clear<HBL2::Component::EditorVisible>();
        m_Registry.storage<HBL2::Component::EditorVisible>().clear();
        m_Registry.compact<HBL2::Component::EditorVisible>();

        m_Registry.clear<HBL2::Component::Transform>();
        m_Registry.storage<HBL2::Component::Transform>().clear();
        m_Registry.compact<HBL2::Component::Transform>();

        m_Registry.clear<HBL2::Component::Link>();
        m_Registry.storage<HBL2::Component::Link>().clear();
        m_Registry.compact<HBL2::Component::Link>();

        m_Registry.clear<HBL2::Component::Camera>();
        m_Registry.storage<HBL2::Component::Camera>().clear();
        m_Registry.compact<HBL2::Component::Camera>();

        m_Registry.clear<HBL2::Component::Sprite_New>();
        m_Registry.storage<HBL2::Component::Sprite_New>().clear();
        m_Registry.compact<HBL2::Component::Sprite_New>();

        m_Registry.clear<HBL2::Component::StaticMesh_New>();
        m_Registry.storage<HBL2::Component::StaticMesh_New>().clear();
        m_Registry.compact<HBL2::Component::StaticMesh_New>();

        m_Registry.clear<HBL2::Component::Light>();
        m_Registry.storage<HBL2::Component::Light>().clear();
        m_Registry.compact<HBL2::Component::Light>();

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

    void Scene::operator=(const HBL2::Scene& other)
    {
        m_Name = other.m_Name;
        m_Systems = other.m_Systems;
        m_CoreSystems = other.m_CoreSystems;
        m_RuntimeSystems = other.m_RuntimeSystems;
        m_EntityMap = other.m_EntityMap;
        m_MetaContext = other.m_MetaContext;
        MainCamera = other.MainCamera;
    }
}
