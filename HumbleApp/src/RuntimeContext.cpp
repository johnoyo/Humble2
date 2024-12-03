#include "RuntimeContext.h"

namespace HBL2
{
	namespace Runtime
	{
		void RuntimeContext::OnCreate()
		{
			HBL2::Context::Mode = HBL2::Mode::Runtime;
			HBL2::AssetManager::Instance = new HBL2::EditorAssetManager;

			OpenProject();

			HBL2::EventDispatcher::Get().Register("SceneChangeEvent", [&](const HBL2::Event& e)
			{
				const HBL2::SceneChangeEvent& sce = dynamic_cast<const HBL2::SceneChangeEvent&>(e);
				m_ActiveScene = HBL2::ResourceManager::Instance->GetScene(sce.NewScene);
			});
		}

		void RuntimeContext::OnUpdate(float ts)
		{
			if (m_ActiveScene == nullptr)
			{
				return;
			}

			for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
			{
				if (system->GetState() == HBL2::SystemState::Play)
				{
					system->OnUpdate(ts);
				}
			}
		}

		void RuntimeContext::OnGuiRender(float ts)
		{
			if (m_ActiveScene == nullptr)
			{
				return;
			}

			for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
			{
				if (system->GetState() == HBL2::SystemState::Play)
				{
					system->OnGuiRender(ts);
				}
			}
		}

		void RuntimeContext::OnDestroy()
		{
			if (m_ActiveScene == nullptr)
			{
				return;
			}

			for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
			{
				system->OnDestroy();
			}
		}

		bool RuntimeContext::OpenProject()
		{
			HBL2::TextureUtilities::Get().LoadWhiteTexture();
			HBL2::ShaderUtilities::Get().LoadBuiltInShaders();

			std::filesystem::path filepath;

			for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::current_path()))
			{
				if (entry.path().extension() == ".hblproj")
				{
					filepath = entry.path();
				}
			}

			if (filepath.empty())
			{
				HBL2_ERROR("Could not find project.");
				return false;
			}

			if (HBL2::Project::Load(std::filesystem::path(filepath)) != nullptr)
			{
				HBL2::Project::OpenStartingScene(true);
				return true;
			}

			HBL2_ERROR("Could not open specified project at path \"{0}\".", filepath);
			HBL2::Window::Instance->Close();

			return false;
		}
	}
}