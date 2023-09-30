#include "Scene.h"

namespace HBL2
{
    Scene::Scene(const std::string& name) : m_Name(name)
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
}