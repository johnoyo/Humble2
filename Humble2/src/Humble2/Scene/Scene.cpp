#include "Scene.h"

#include "Project\Project.h"
#include "SceneSerializer.h"

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
    }

    Scene* Scene::Copy(Scene* other)
    {
        HBL2_FUNC_PROFILE();

        Scene* newScene = new Scene({ .name = other->m_Name + "(Clone)"});

        SceneSerializer serializer(newScene);
        serializer.Deserialize(Project::GetAssetFileSystemPath(std::filesystem::path("Scenes") / std::filesystem::path("EmptyScene.humble")));

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
        for (ISystem* system : other->m_Systems)
        {
            newScene->RegisterSystem(system);
        }

        newScene->MainCamera = other->MainCamera;

        return newScene;
    }

    HBL2::Scene& Scene::operator=(const HBL2::Scene& other)
    {
        m_Name = other.GetName();
        return (HBL2::Scene&)other;
    }
}
