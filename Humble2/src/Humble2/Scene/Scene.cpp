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

        // Clone dll user systems.
        for (ISystem* system : src->m_Systems)
        {
            if (system->GetType() == SystemType::User)
            {
                const std::string& dllPath = "assets\\dlls\\" + system->Name + "\\" + system->Name + ".dll";
                ISystem* newSystem = NativeScriptUtilities::Get().LoadSystem(dllPath, dst);
            }
        }

        // Set main camera.
        dst->MainCamera = src->MainCamera;
        dst->m_MetaContext = src->m_MetaContext;
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

        if (system->GetType() == SystemType::User)
        {
            NativeScriptUtilities::Get().UnloadSystem(system->Name, this);
        }
        else
        {
            delete system;
        }
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
