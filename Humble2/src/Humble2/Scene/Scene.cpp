#include "Scene.h"

#include "SceneSerializer.h"

namespace HBL2
{
    Scene::Scene(const SceneDescriptor& desc) : m_Name(desc.name)
    {
        if (!desc.path.empty())
        {
            SceneSerializer sceneSerializer(this);
            sceneSerializer.Deserialize(desc.path);
        }
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
        // TODO: Do an actual deep copy.
        return other;
    }

    HBL2::Scene& Scene::operator=(const HBL2::Scene& other)
    {
        // TODO: insert return statement here
        return (HBL2::Scene&)other;
    }
}