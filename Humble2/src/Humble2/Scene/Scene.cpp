#include "Scene.h"

namespace HBL2
{
    Scene::Scene(const std::string& name) : m_Name(name)
    {
        
    }

    Scene::~Scene()
    {
    }

    Scene* Scene::Copy(Scene* other)
    {
        // TODO: Do an actual deep copy.
        return other;
    }
}