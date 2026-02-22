#include "SoundSystem.h"

#include <Project\Project.h>
#include <Sound\FmodSoundEngine.h>

namespace HBL2
{
    static constexpr uint32_t InvalidIndex = UINT32_MAX;

    SoundSystem::SoundSystem()
    {
        Name = "SoundSystem";
    }

    void SoundSystem::OnCreate()
    {
        const auto& projectSettings = Project::GetActive()->GetSpecification().Settings;

        // Pick implementation.
        switch (projectSettings.SoundImpl)
        {
        case SoundEngineImpl::FMOD:
        case SoundEngineImpl::CUSTOM:
            SoundEngine::Instance = new FmodSoundEngine;
            break;
        }

        SoundEngineConfig cfg{};
        cfg.MaxChannels = 512;
        cfg.Enable3D = true;

        if (!SoundEngine::Instance || !SoundEngine::Instance->Initialize(m_Context, cfg))
        {
            return;
        }

        m_Channels.reserve(256);

        // Pre-load sounds referenced by existing AudioSource components.
        m_Context->View<Component::AudioSource>()
            .Each([&](Component::AudioSource& audioSource)
            {
                if (!audioSource.Sound.IsValid())
                {
                    return;
                }

                Sound* sound = ResourceManager::Instance->GetSound(audioSource.Sound);
                if (!sound)
                {
                    return;
                }

                if (sound->ID == InvalidSoundID)
                {
                    const auto absPath = Project::GetAssetFileSystemPath(sound->Path);
                    SoundLoadFlags flags = SoundLoadFlags::Compressed | SoundLoadFlags::NonBlocking | SoundLoadFlags::Spatial3D;
                    sound->ID = SoundEngine::Instance->LoadSound(absPath, sound->Name.c_str(), flags);
                }
            });

        m_Initialized = true;
    }

    void SoundSystem::OnUpdate(float ts)
    {
        BEGIN_PROFILE_SYSTEM();

        if (!m_Initialized || !SoundEngine::Instance)
        {
            END_PROFILE_SYSTEM(RunningTime);
            return;
        }

        UpdateListener();

        m_Context->View<Component::AudioSource>()
            .Each([&](Entity entity, Component::AudioSource& audioSource)
            {
                if (!audioSource.Sound.IsValid())
                {
                    return;
                }

                Sound* sound = ResourceManager::Instance->GetSound(audioSource.Sound);
                if (!sound)
                {
                    return;
                }

                // Lazy-load for runtime-added sources
                if (sound->ID == InvalidSoundID)
                {
                    const auto absPath = Project::GetAssetFileSystemPath(sound->Path);
                    SoundLoadFlags flags = SoundLoadFlags::Compressed | SoundLoadFlags::NonBlocking | SoundLoadFlags::Spatial3D;
                    sound->ID = SoundEngine::Instance->LoadSound(absPath, sound->Name.c_str(), flags);
                    if (sound->ID == InvalidSoundID)
                    {
                        return;
                    }
                }

                bool isValid = (audioSource.ChannelIndex != InvalidIndex && audioSource.ChannelIndex < m_Channels.size());
                ChannelEntry* entry = isValid ? &m_Channels[audioSource.ChannelIndex] : nullptr;

                // If the channel finished, clear it
                if (entry && entry->channel != SoundEngine::InvalidChannel && !SoundEngine::Instance->IsValid(entry->channel))
                {
                    entry->channel = SoundEngine::InvalidChannel;
                    audioSource.ChannelIndex = InvalidIndex;
                    entry = nullptr;
                }

                switch (audioSource.State)
                {
                case Component::AudioSource::PlaybackState::Playing:
                {
                    if (!entry || entry->channel == SoundEngine::InvalidChannel)
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
                    if (!entry || entry->channel == SoundEngine::InvalidChannel)
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
                    if (entry && entry->channel != SoundEngine::InvalidChannel)
                    {
                        StopChannel(*entry, audioSource.ChannelIndex);
                        audioSource.ChannelIndex = InvalidIndex;
                    }
                    break;
                }
                case Component::AudioSource::PlaybackState::Trigger:
                {
                    if (entry && entry->channel != SoundEngine::InvalidChannel)
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
                    if (entry && entry->channel != SoundEngine::InvalidChannel)
                    {
                        PauseChannel(*entry, false);
                    }
                    else
                    {
                        StartChannel(audioSource, entity, false);
                    }

                    audioSource.State = Component::AudioSource::PlaybackState::Playing;
                    break;
                }
                }

                // Live parameter update
                isValid = (audioSource.ChannelIndex != InvalidIndex && audioSource.ChannelIndex < m_Channels.size());
                entry = isValid ? &m_Channels[audioSource.ChannelIndex] : nullptr;

                if (entry && entry->channel != SoundEngine::InvalidChannel && audioSource.State != Component::AudioSource::PlaybackState::Stopped)
                {
                    UpdateChannelParams(audioSource, *entry);
                }
            });

        SoundEngine::Instance->Update();

        END_PROFILE_SYSTEM(RunningTime);
    }

    void SoundSystem::OnDestroy()
    {
        if (!m_Initialized)
        {
            return;
        }

        // Stop everything
        if (SoundEngine::Instance)
        {
            for (ChannelEntry& e : m_Channels)
            {
                if (e.channel != SoundEngine::InvalidChannel)
                {
                    SoundEngine::Instance->StopChannel(e.channel);
                    e.channel = SoundEngine::InvalidChannel;
                }
            }
        }

        // Release sounds referenced by AudioSources.
        std::unordered_set<SoundID> released;

        m_Context->View<Component::AudioSource>()
            .Each([&](Component::AudioSource& audioSource)
            {
                Sound* sound = ResourceManager::Instance->GetSound(audioSource.Sound);
                if (!sound)
                {
                    return;
                }

                if (sound->ID != InvalidSoundID && SoundEngine::Instance)
                {
                    if (released.insert(sound->ID).second)
                    {
                        SoundEngine::Instance->ReleaseSound(sound->ID);
                    }

                    sound->ID = InvalidSoundID;
                }
            });

        if (SoundEngine::Instance)
        {
            SoundEngine::Instance->Shutdown();
            delete SoundEngine::Instance;
            SoundEngine::Instance = nullptr;
        }

        m_Channels.clear();
        m_Free = {};
        m_Initialized = false;
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
        return (uint32_t)m_Channels.size() - 1;
    }

    void SoundSystem::FreeSlot(uint32_t idx)
    {
        if (idx >= m_Channels.size())
        {
            return;
        }

        ChannelEntry& e = m_Channels[idx];
        e.channel = SoundEngine::InvalidChannel;
        e.owner = Entity::Null;

        m_Free.push(idx);
    }

    void SoundSystem::StartChannel(Component::AudioSource& src, Entity entity, bool paused)
    {
        Sound* sound = ResourceManager::Instance->GetSound(src.Sound);
        if (!sound || sound->ID == InvalidSoundID || !SoundEngine::Instance)
        {
            return;
        }

        const uint32_t slot = AllocateSlot();
        ChannelEntry& e = m_Channels[slot];

        e.owner = entity;
        src.ChannelIndex = slot;

        // Start paused so we can configure first
        e.channel = SoundEngine::Instance->PlaySound(sound->ID, true);
        if (e.channel == SoundEngine::InvalidChannel)
        {
            FreeSlot(slot);
            src.ChannelIndex = InvalidIndex;
            return;
        }

        SoundEngine::Instance->SetVolume(e.channel, src.Volume);
        SoundEngine::Instance->SetPitch(e.channel, src.Pitch);
        SoundEngine::Instance->SetLooping(e.channel, (src.Flags & Component::AudioSource::Looping) != 0);

        const bool spatial = (src.Flags & Component::AudioSource::Spatialised) != 0;
        SoundEngine::Instance->Set3DEnabled(e.channel, spatial);

        if (spatial)
        {
            if (m_Context->HasComponent<Component::Transform>(entity))
            {
                auto& tr = m_Context->GetComponent<Component::Transform>(entity);
                Update3D(e, tr);
            }
            else
            {
                SoundEngine::Instance->Set3DEnabled(e.channel, false);
            }
        }

        SoundEngine::Instance->SetPaused(e.channel, paused);
    }

    void SoundSystem::StopChannel(ChannelEntry& e, uint32_t slotIndex)
    {
        if (SoundEngine::Instance && e.channel != SoundEngine::InvalidChannel)
        {
            SoundEngine::Instance->StopChannel(e.channel);
            e.channel = SoundEngine::InvalidChannel;
        }

        FreeSlot(slotIndex);
    }

    void SoundSystem::PauseChannel(ChannelEntry& e, bool paused)
    {
        if (SoundEngine::Instance && e.channel != SoundEngine::InvalidChannel)
        {
            SoundEngine::Instance->SetPaused(e.channel, paused);
        }
    }

    void SoundSystem::UpdateChannelParams(const Component::AudioSource& src, ChannelEntry& e)
    {
        if (!SoundEngine::Instance || e.channel == SoundEngine::InvalidChannel)
        {
            return;
        }

        SoundEngine::Instance->SetVolume(e.channel, src.Volume);
        SoundEngine::Instance->SetPitch(e.channel, src.Pitch);
        SoundEngine::Instance->SetLooping(e.channel, (src.Flags & Component::AudioSource::Looping) != 0);

        if ((src.Flags & Component::AudioSource::Spatialised) && m_Context->HasComponent<Component::Transform>(e.owner))
        {
            auto& tr = m_Context->GetComponent<Component::Transform>(e.owner);
            Update3D(e, tr);
        }
    }

    void SoundSystem::Update3D(ChannelEntry& e, const Component::Transform& tr)
    {
        glm::vec3 worldPosition = tr.WorldMatrix * glm::vec4(tr.Translation, 1.0f);

        Source3D s{};
        s.Position = worldPosition;
        s.Velocity = { 0, 0, 0 };

        SoundEngine::Instance->Set3DAttributes(e.channel, s);
    }

    void SoundSystem::UpdateListener()
    {
        if (!SoundEngine::Instance)
        {
            return;
        }

        if (m_Context->MainCamera == Entity::Null)
        {
            return;
        }

        if (!m_Context->HasComponent<Component::AudioListener>(m_Context->MainCamera))
        {
            return;
        }

        auto& listener = m_Context->GetComponent<Component::AudioListener>(m_Context->MainCamera);
        if (!listener.Enabled)
        {
            return;
        }

        if (!m_Context->HasComponent<Component::Transform>(m_Context->MainCamera))
        {
            return;
        }

        auto& tr = m_Context->GetComponent<Component::Transform>(m_Context->MainCamera);

        glm::vec3 worldPosition = tr.WorldMatrix * glm::vec4(tr.Translation, 1.0f);

        Listener3D l{};
        l.Position = worldPosition;
        l.Velocity = { 0, 0, 0 };
        l.Forward = { 0.f, 0.f, 1.f };
        l.Up = { 0.f, 1.f, 0.f };

        SoundEngine::Instance->SetListener(0, l);
    }
}
