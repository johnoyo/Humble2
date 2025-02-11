#include "SoundManager.h"

namespace HBL2
{
    SoundManager* SoundManager::s_Instance = nullptr;

    void SoundManager::Initialize()
    {
        HBL2_CORE_ASSERT(s_Instance == nullptr, "SoundManager::s_Instance is not null! SoundManager::Initialize has been called twice.");
        s_Instance = new SoundManager;

        Get().Start();
    }

    void SoundManager::Shutdown()
    {
        for (auto const& sound : Get().m_Sounds)
        {
            sound.second->release();
        }

        HBL2_CORE_ASSERT(s_Instance != nullptr, "SoundManager::s_Instance is null!");
        delete s_Instance;
        s_Instance = nullptr;
    }

    void SoundManager::Update()
    {
        m_SoundSystem->update();
    }

	void SoundManager::Play(const std::string& source, bool playLooped, bool startPaused)
	{
        // Create the sound if it does not exist.
        if (!Exists(source))
        {
            FMOD::Sound* sound = nullptr;
            m_Result = m_SoundSystem->createSound(source.c_str(), FMOD_DEFAULT, nullptr, &sound);
            if (!SucceededOrWarn("FMOD: Failed to create sound", m_Result))
            {
                return;
            }

            if (playLooped)
            {
                sound->setMode(FMOD_LOOP_NORMAL);
            }
            else
            {
                sound->setMode(FMOD_LOOP_OFF);
            }

            m_Sounds[source] = sound;
        }

        // Play the sound.
        m_Result = m_SoundSystem->playSound(m_Sounds[source], nullptr, startPaused, &m_Channels[source]);

        if (!SucceededOrWarn("FMOD: Failed to play sound", m_Result))
        {
            return;
        }
	}

    void SoundManager::Stop(const std::string& source)
    {
        // Stop all sounds.
        if (ChannelExists(source))
        {
            m_Result = m_Channels[source]->stop();
            if (!SucceededOrWarn("FMOD: Failed to stop sound", m_Result))
            {
                return;
            }
        }
    }

    void SoundManager::Start()
    {
        // Create the main system object.
        m_Result = FMOD::System_Create(&m_SoundSystem);
        if (m_Result != FMOD_OK)
        {
            HBL2_CORE_ERROR("FMOD error! ({}) {}", m_Result, FMOD_ErrorString(m_Result));
            return;
        }

        // Initialize FMOD.
        m_Result = m_SoundSystem->init(512, FMOD_INIT_NORMAL, 0);
        if (m_Result != FMOD_OK)
        {
            HBL2_CORE_ERROR("FMOD error! ({}) {}", m_Result, FMOD_ErrorString(m_Result));
            return;
        }

        // Create the channel group.
        m_Result = m_SoundSystem->createChannelGroup("inGameSoundEffects", &m_ChannelGroup);
        if (!SucceededOrWarn("FMOD: Failed to create in-game sound effects channel group", m_Result))
        {
            return;
        }
    }

    bool SoundManager::Exists(const std::string& soundName)
    {
        return (m_Sounds.find(soundName) != m_Sounds.end());
    }

    bool SoundManager::ChannelExists(const std::string& soundName)
    {
        return (m_Channels.find(soundName) != m_Channels.end());
    }

    bool SoundManager::SucceededOrWarn(const std::string& message, FMOD_RESULT result)
    {
        if (result != FMOD_OK)
        {
            HBL2_CORE_ERROR("{}: {} {}", message, result, FMOD_ErrorString(result));
            return false;
        }
        return true;
    }
}
