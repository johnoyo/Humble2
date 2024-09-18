#include "Scene.h"

#include "Project\Project.h"
#include "SceneSerializer.h"

#include "Systems\TransformSystem.h"
#include "Systems\LinkSystem.h"
#include "Systems\CameraSystem.h"
#include "Systems\StaticMeshRenderingSystem.h"
#include "Systems\SpriteRenderingSystem.h"

namespace HBL2
{
    Scene::Scene(const SceneDescriptor&& desc) : m_Name(desc.name)
    {
    }

    Scene::~Scene()
    {
        m_Registry.clear();

        for (ISystem* system : m_Systems)
        {
            delete system;
        }

        m_Systems.clear();
        m_CoreSystems.clear();
        m_RuntimeSystems.clear();
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

        SceneSerializer serializer(dst);
        serializer.Deserialize(Project::GetAssetFileSystemPath(std::filesystem::path("Scenes") / (src->GetName() + ".humble")));

        //// Clone entites
        //std::unordered_map<UUID, entt::entity> enttMap;
        //other->m_Registry
        //    .view<Component::ID>()
        //    .each([&](entt::entity entity, Component::ID& id)
        //    {
        //        const auto& name = other->m_Registry.get<Component::Tag>(entity).Name;
        //        entt::entity newEntity = newScene->CreateEntityWithUUID(id.Identifier, name);
        //        enttMap[id.Identifier] = newEntity;
        //    });

        //// Clone components
        //{
        //    auto view = other->m_Registry.view<Component::Transform>();
        //    newScene->m_Registry.insert(other->m_Registry.data(), other->m_Registry.data() + other->m_Registry.size(), view);
        //}

        //{
        //    auto view = other->m_Registry.view<Component::Link>();
        //    newScene->m_Registry.insert(other->m_Registry.data(), other->m_Registry.data() + other->m_Registry.size(), view);
        //}

        //{
        //    auto view = other->m_Registry.view<Component::Camera>();
        //    newScene->m_Registry.insert(other->m_Registry.data(), other->m_Registry.data() + other->m_Registry.size(), view);
        //}

        //{
        //    auto view = other->m_Registry.view<Component::StaticMesh_New>();
        //    newScene->m_Registry.insert(other->m_Registry.data(), other->m_Registry.data() + other->m_Registry.size(), view);
        //}

        //{
        //    auto view = other->m_Registry.view<Component::Sprite_New>();
        //    newScene->m_Registry.insert(other->m_Registry.data(), other->m_Registry.data() + other->m_Registry.size(), view);
        //}

        // Clone systems
        dst->RegisterSystem(new TransformSystem);
        dst->RegisterSystem(new LinkSystem);
        dst->RegisterSystem(new CameraSystem, SystemType::Runtime);
        dst->RegisterSystem(new StaticMeshRenderingSystem);
        dst->RegisterSystem(new SpriteRenderingSystem);

        dst->MainCamera = src->MainCamera;
    }

    void Scene::operator=(const HBL2::Scene& other)
    {
        m_Name = other.m_Name;
        m_Systems = other.m_Systems;
        m_CoreSystems = other.m_CoreSystems;
        m_RuntimeSystems = other.m_RuntimeSystems;
        m_EntityMap = other.m_EntityMap;
        MainCamera = other.MainCamera;

        // Registry cannot be cloned automatically.
        m_Registry.clear();
    }
}
