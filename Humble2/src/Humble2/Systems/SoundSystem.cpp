#include "SoundSystem.h"

#include <Project\Project.h>

namespace HBL2
{
    #define FMOD_CHECK(expr)                                                     \
        do                                                                       \
        {                                                                        \
            FMOD_RESULT _res = (expr);                                           \
            if (_res != FMOD_OK)                                                 \
            {                                                                    \
                HBL2_CORE_ERROR("[FMOD] {} : {}", #expr, FMOD_ErrorString(_res));\
            }                                                                    \
        } while (false)                                                          \
    
    constexpr uint32_t InvalidIndex = UINT32_MAX;

    inline FMOD_VECTOR ToFmodVec(const glm::vec3& v)
    {
        return FMOD_VECTOR{ v.x, v.y, v.z };
    }

    SoundSystem::SoundSystem()
    {
        Name = "SoundSystem";
    }

    void SoundSystem::OnCreate()
    {
        // Create the main system object.
        FMOD_CHECK(FMOD::System_Create(&m_SoundSystem));

        if (m_SoundSystem == nullptr)
        {
            return;
        }

        // Initialize FMOD.
        FMOD_CHECK(m_SoundSystem->init(512, FMOD_INIT_NORMAL, 0));

        // Create the channel group.
        FMOD_CHECK(m_SoundSystem->createChannelGroup("InGameSoundEffects", &m_ChannelGroup));

        m_Channels.reserve(256);

        m_Context->View<Component::AudioSource>()
            .Each([&](Component::AudioSource& audioSource)
            {
                if (!audioSource.Sound.IsValid())
                {
                    return;
                }

                Sound* sound = ResourceManager::Instance->GetSound(audioSource.Sound);

                if (sound->Instance == nullptr)
                {
                    static unsigned kDefaultMode = FMOD_DEFAULT | FMOD_CREATECOMPRESSEDSAMPLE | FMOD_NONBLOCKING | FMOD_3D;
                    const std::string& finalPath = Project::GetAssetFileSystemPath(sound->Path).string();
                    FMOD_CHECK(m_SoundSystem->createSound(finalPath.c_str(), kDefaultMode, nullptr, &sound->Instance));
                }
            });
    }

    void SoundSystem::OnUpdate(float ts)
    {
        m_Context->View<Component::AudioSource>()
            .Each([&](Entity entity, Component::AudioSource& audioSource)
            {
                if (!audioSource.Sound.IsValid())
                {
                    return;
                }

                ChannelEntry* entry = (audioSource.ChannelIndex != UINT32_MAX) ? &m_Channels[audioSource.ChannelIndex] : nullptr;

                switch (audioSource.State)
                {
                case Component::AudioSource::PlaybackState::Playing:
                    {
                        if (!entry || !entry->channel)
                        {
                            StartChannel(audioSource, entity, false);
                        }
                        else
                        {
                            PauseChannel(*entry, false);
                        }
                        break;
                    }
                case Component::AudioSource::PlaybackState::Paused:
                    {
                        if (!entry || !entry->channel)
                        {
                            StartChannel(audioSource, entity, true);
                        }
                        else
                        {
                            PauseChannel(*entry, true);
                        }
                        break;
                    }
                case Component::AudioSource::PlaybackState::Stopped:
                    {
                        if (entry && entry->channel)
                        {
                            StopChannel(*entry, audioSource.ChannelIndex);
                            audioSource.ChannelIndex = InvalidIndex;
                        }
                        break;
                    }
                case Component::AudioSource::PlaybackState::Trigger:
                {
                    if (entry && entry->channel)
                    {
                        StopChannel(*entry, audioSource.ChannelIndex);
                        audioSource.ChannelIndex = InvalidIndex;
                    }
                    StartChannel(audioSource, entity, false);
                    audioSource.State = Component::AudioSource::PlaybackState::Playing;
                    break;
                }
                case Component::AudioSource::PlaybackState::Resume:
                {
                    // If we already have a channel then just un-pause it
                    if (entry && entry->channel)
                    {
                        PauseChannel(*entry, false);
                    }
                    else
                    {
                        // The clip had never played or it finished while paused
                        StartChannel(audioSource, entity, false);
                    }
                    audioSource.State = Component::AudioSource::PlaybackState::Playing;   // auto-clear
                    break;
                }
                }

                // Live parameter update (volume / pitch / 3-D)
                if (entry && entry->channel && audioSource.State != Component::AudioSource::PlaybackState::Stopped)
                {
                    UpdateChannelParams(audioSource, *entry);
                }
            });

        m_SoundSystem->update();
    }

    void SoundSystem::OnDestroy()
    {
        // Stop everything.
        for (ChannelEntry& e : m_Channels)
        {
            if (e.channel)
            {
                e.channel->stop();
                e.channel = nullptr;
            }
        }

        // Release sounds.
        m_Context->View<Component::AudioSource>()
            .Each([&](Component::AudioSource& audioSource)
            {
                Sound* sound = ResourceManager::Instance->GetSound(audioSource.Sound);

                if (sound == nullptr)
                {
                    return;
                }

                if (sound->Instance != nullptr)
                {
                    sound->Instance->release();
                    sound->Instance = nullptr;
                }
            });

        FMOD_CHECK(m_ChannelGroup ? m_ChannelGroup->release() : FMOD_OK);
        FMOD_CHECK(m_SoundSystem ? m_SoundSystem->close() : FMOD_OK);
        FMOD_CHECK(m_SoundSystem ? m_SoundSystem->release() : FMOD_OK);

        m_Channels.clear();
        m_Free = {};
    }

    void SoundSystem::Play(Component::AudioSource& audioSource)
    {
        audioSource.State = Component::AudioSource::PlaybackState::Trigger;
    }

    void SoundSystem::Pause(Component::AudioSource& audioSource)
    {
        audioSource.State = Component::AudioSource::PlaybackState::Paused;
    }

    void SoundSystem::Resume(Component::AudioSource& audioSource)
    {
        audioSource.State = Component::AudioSource::PlaybackState::Resume;
    }

    void SoundSystem::Stop(Component::AudioSource& audioSource)
    {
        audioSource.State = Component::AudioSource::PlaybackState::Stopped;
    }

    uint32_t SoundSystem::AllocateSlot()
    {
        if (!m_Free.empty())
        {
            uint32_t idx = m_Free.front();
            m_Free.pop();
            return idx;
        }

        m_Channels.emplace_back();
        return static_cast<uint32_t>(m_Channels.size() - 1);
    }

    void SoundSystem::FreeSlot(uint32_t idx)
    {
        if (idx >= m_Channels.size())
        {
            return;
        }

        ChannelEntry& e = m_Channels[idx];
        e.channel = nullptr;
        e.owner = Entity::Null;

        m_Free.push(idx);
    }

    void SoundSystem::StartChannel(Component::AudioSource& src, uint32_t entity, bool paused)
    {
        // Resolve the asset
        Sound* sound = ResourceManager::Instance->GetSound(src.Sound);

        if (!sound)
        {
            return;
        }

        const uint32_t slot = AllocateSlot();
        ChannelEntry& e = m_Channels[slot];

        e.owner = entity;
        src.ChannelIndex = slot;

        FMOD_CHECK(m_SoundSystem->playSound(sound->Instance, m_ChannelGroup, true, &e.channel));

        // Basic parameters
        e.channel->setVolume(src.Volume);
        e.channel->setPitch(src.Pitch);

        if (src.Flags & Component::AudioSource::AudioFlags::Looping)
        {
            e.channel->setMode(FMOD_LOOP_NORMAL);
        }

        // 3-D?
        if (src.Flags & Component::AudioSource::AudioFlags::Spatialised)
        {
            if (m_Context->HasComponent<Component::Transform>(entity))
            {
                auto& tr = m_Context->GetComponent<Component::Transform>(entity);
                Update3D(e, tr);
            }
            else
            {
                // No transform then treat as 2-D
                e.channel->setMode(FMOD_2D);
            }
        }
        else
        {
            e.channel->setMode(FMOD_2D);
        }

        e.channel->setPaused(paused);
    }

    void SoundSystem::StopChannel(ChannelEntry& e, uint32_t slotIndex)
    {
        if (e.channel)
        {
            e.channel->stop();
            e.channel = nullptr;
        }

        FreeSlot(slotIndex);
    }

    void SoundSystem::PauseChannel(ChannelEntry& e, bool paused)
    {
        if (e.channel)
        {
            e.channel->setPaused(paused);
        }
    }

    void SoundSystem::UpdateChannelParams(const Component::AudioSource& src, ChannelEntry& e)
    {
        e.channel->setVolume(src.Volume);
        e.channel->setPitch(src.Pitch);

        if ((src.Flags & Component::AudioSource::AudioFlags::Spatialised) && m_Context->HasComponent<Component::Transform>(e.owner))
        {
            auto& tr = m_Context->GetComponent<Component::Transform>(e.owner);
            Update3D(e, tr);
        }
    }

    void SoundSystem::Update3D(ChannelEntry& e, const Component::Transform& tr)
    {
        glm::vec3 worldPosition = tr.WorldMatrix * glm::vec4(tr.Translation, 1.0f);

        FMOD_VECTOR pos = ToFmodVec(worldPosition);
        FMOD_VECTOR vel = { 0, 0, 0 };

        e.channel->set3DAttributes(&pos, &vel);
    }

    void SoundSystem::UpdateListener()
    {
        if (m_Context->MainCamera == Entity::Null)
        {
            return;
        }

        if (!m_Context->HasComponent<Component::AudioListener>(m_Context->MainCamera))
        {
            return;
        }

        auto& listerner = m_Context->GetComponent<Component::AudioListener>(m_Context->MainCamera);
        auto& tr = m_Context->GetComponent<Component::Transform>(m_Context->MainCamera);

        glm::vec3 worldPosition = tr.WorldMatrix * glm::vec4(tr.Translation, 1.0f);

        FMOD_VECTOR pos = ToFmodVec(worldPosition);
        FMOD_VECTOR vel = { 0, 0, 0 };
        FMOD_VECTOR fwd = ToFmodVec({ 0.f, 0.f, 1.f });
        FMOD_VECTOR up = ToFmodVec({ 0.f, 1.f, 0.f });

        FMOD_CHECK(m_SoundSystem->set3DListenerAttributes(0, &pos, &vel, &fwd, &up));
    }
}
