#include "Scene.h"

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
        // TODO: Do an actual deep copy.
        return other;
    }

    HBL2::Scene& Scene::operator=(const HBL2::Scene& other)
    {
        m_Name = other.GetName();
        return (HBL2::Scene&)other;
    }
}