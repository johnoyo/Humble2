#include "SoundSystem.h"

namespace HBL2
{
	void SoundSystem::OnCreate()
	{
        // Create the main system object.
        FMOD_RESULT result = FMOD::System_Create(&m_SoundSystem);
        if (result != FMOD_OK)
        {
            HBL2_CORE_ERROR("FMOD error! ({}) {}", result, FMOD_ErrorString(result));
            return;
        }

        // Initialize FMOD.
        result = m_SoundSystem->init(512, FMOD_INIT_NORMAL, 0);
        if (result != FMOD_OK)
        {
            HBL2_CORE_ERROR("FMOD error! ({}) {}", result, FMOD_ErrorString(result));
            return;
        }

        // Create the channel group.
        result = m_SoundSystem->createChannelGroup("inGameSoundEffects", &m_ChannelGroup);
        if (!SucceededOrWarn("[FMOD] Failed to create in-game sound effects channel group", result))
        {
            return;
        }

        m_Context->GetRegistry()
            .view<Component::SoundSource>()
            .each([&](entt::entity entity, Component::SoundSource& soundSource)
            {
                Sound* sound = ResourceManager::Instance->GetSound(soundSource.Sound);

                if (sound != nullptr)
                {
                    sound->Create(m_SoundSystem);
                }
            });
	}

	void SoundSystem::OnUpdate(float ts)
	{
        m_Context->GetRegistry()
            .view<Component::SoundSource>()
            .each([&](entt::entity entity, Component::SoundSource& soundSource)
            {
                Sound* sound = ResourceManager::Instance->GetSound(soundSource.Sound);

                if (soundSource.Enabled)
                {
                    if (sound != nullptr)
                    {
                        sound->Play(m_SoundSystem);
                    }
                }
                else
                {
                    if (sound != nullptr)
                    {
                        sound->Stop();
                    }
                }
            });

        m_SoundSystem->update();
	}

    void SoundSystem::OnDestroy()
    {
        m_Context->GetRegistry()
            .view<Component::SoundSource>()
            .each([&](entt::entity entity, Component::SoundSource& soundSource)
            {
                Sound* sound = ResourceManager::Instance->GetSound(soundSource.Sound);

                if (sound != nullptr)
                {
                    sound->Destroy();
                }
            });

        if (m_SoundSystem != nullptr)
        {

            FMOD_RESULT result = m_SoundSystem->close();
            if (result != FMOD_OK)
            {
                HBL2_CORE_ERROR("FMOD error! ({}) {}", result, FMOD_ErrorString(result));
                return;
            }

            result = m_SoundSystem->release();
            if (result != FMOD_OK)
            {
                HBL2_CORE_ERROR("FMOD error! ({}) {}", result, FMOD_ErrorString(result));
                return;
            }
        }
    }

    bool SoundSystem::SucceededOrWarn(const std::string& message, FMOD_RESULT result)
    {
        if (result != FMOD_OK)
        {
            HBL2_CORE_ERROR("{}: {} {}", message, result, FMOD_ErrorString(result));
            return false;
        }
        return true;
    }
}
