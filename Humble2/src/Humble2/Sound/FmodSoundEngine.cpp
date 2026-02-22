#include "FmodSoundEngine.h"

namespace HBL2
{
    #define FMOD_CHECK(expr)                                                    \
    do                                                                          \
    {                                                                           \
        FMOD_RESULT _res = (expr);                                              \
        if (_res != FMOD_OK)                                                    \
        {                                                                       \
            HBL2_CORE_ERROR("[FMOD] {} : {}", #expr, FMOD_ErrorString(_res));   \
        }                                                                       \
    } while (false)

    SoundID FmodSoundEngine::NewID()
    {
        static std::atomic<uint64_t> s_Next{ 1 };
        return s_Next.fetch_add(1, std::memory_order_relaxed);
    }

    bool FmodSoundEngine::Initialize(void* /*engineContext*/, const SoundEngineConfig& cfg)
    {
        FMOD_CHECK(FMOD::System_Create(&m_System));
        if (!m_System)
        {
            return false;
        }

        FMOD_CHECK(m_System->init((int)cfg.MaxChannels, FMOD_INIT_NORMAL, nullptr));
        FMOD_CHECK(m_System->createChannelGroup("InGameSoundEffects", &m_Group));

        return (m_Group != nullptr);
    }

    void FmodSoundEngine::Shutdown()
    {
        if (m_Group)
        {
            m_Group->stop();
        }

        for (auto& [id, snd] : m_Sounds)
        {
            if (snd)
            {
                snd->release();
            }
        }
        m_Sounds.clear();

        FMOD_CHECK(m_Group ? m_Group->release() : FMOD_OK);
        m_Group = nullptr;

        FMOD_CHECK(m_System ? m_System->close() : FMOD_OK);
        FMOD_CHECK(m_System ? m_System->release() : FMOD_OK);
        m_System = nullptr;
    }

    SoundID FmodSoundEngine::LoadSound(const std::filesystem::path& absolutePath, const char* debugName, SoundLoadFlags flags)
    {
        if (!m_System)
        {
            return InvalidSoundID;
        }

        unsigned mode = FMOD_DEFAULT;

        if (HasFlag(flags, SoundLoadFlags::Compressed))  mode |= FMOD_CREATECOMPRESSEDSAMPLE;
        if (HasFlag(flags, SoundLoadFlags::NonBlocking)) mode |= FMOD_NONBLOCKING;
        if (HasFlag(flags, SoundLoadFlags::Stream))      mode |= FMOD_CREATESTREAM;

        // Allow 3D usage if requested (channel can still be switched to 2D later)
        if (HasFlag(flags, SoundLoadFlags::Spatial3D)) mode |= FMOD_3D;
        else                                          mode |= FMOD_2D;

        FMOD::Sound* sound = nullptr;
        const std::string p = absolutePath.string();
        FMOD_CHECK(m_System->createSound(p.c_str(), mode, nullptr, &sound));

        if (!sound)
        {
            return InvalidSoundID;
        }

        const SoundID id = NewID();
        m_Sounds.emplace(id, sound);
        return id;
    }

    void FmodSoundEngine::ReleaseSound(SoundID id)
    {
        auto it = m_Sounds.find(id);
        if (it == m_Sounds.end())
        {
            return;
        }

        if (it->second)
        {
            it->second->release();
        }

        m_Sounds.erase(it);
    }

    SoundEngine::ChannelHandle FmodSoundEngine::PlaySound(SoundID id, bool startPaused)
    {
        if (!m_System)
        {
            return InvalidChannel;
        }

        auto it = m_Sounds.find(id);
        if (it == m_Sounds.end() || !it->second)
        {
            return InvalidChannel;
        }

        FMOD::Channel* ch = nullptr;
        FMOD_CHECK(m_System->playSound(it->second, m_Group, startPaused, &ch));
        if (!ch)
        {
            return InvalidChannel;
        }

        return ToHandle(ch);
    }

    void FmodSoundEngine::StopChannel(ChannelHandle ch)
    {
        FMOD::Channel* c = FromHandle(ch);
        if (!c)
        {
            return;
        }

        c->stop();
    }

    void FmodSoundEngine::SetPaused(ChannelHandle ch, bool paused)
    {
        FMOD::Channel* c = FromHandle(ch);
        if (!c)
        {
            return;
        }

        c->setPaused(paused);
    }

    bool FmodSoundEngine::IsValid(ChannelHandle ch) const
    {
        FMOD::Channel* c = FromHandle(ch);
        if (!c)
        {
            return false;
        }

        bool playing = false;
        FMOD_RESULT r = c->isPlaying(&playing);
        if (r != FMOD_OK)
        {
            return false;
        }

        return playing;
    }

    void FmodSoundEngine::SetVolume(ChannelHandle ch, float volume01)
    {
        FMOD::Channel* c = FromHandle(ch);
        if (!c)
        {
            return;
        }

        c->setVolume(volume01);
    }

    void FmodSoundEngine::SetPitch(ChannelHandle ch, float pitch)
    {
        FMOD::Channel* c = FromHandle(ch);
        if (!c)
        {
            return;
        }

        c->setPitch(pitch);
    }

    void FmodSoundEngine::SetLooping(ChannelHandle ch, bool looping)
    {
        FMOD::Channel* c = FromHandle(ch);
        if (!c)
        {
            return;
        }

        c->setMode(looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
    }

    void FmodSoundEngine::Set3DEnabled(ChannelHandle ch, bool enabled)
    {
        FMOD::Channel* c = FromHandle(ch);
        if (!c)
        {
            return;
        }

        c->setMode(enabled ? FMOD_3D : FMOD_2D);
    }

    void FmodSoundEngine::Set3DAttributes(ChannelHandle ch, const Source3D& src3d)
    {
        FMOD::Channel* c = FromHandle(ch);
        if (!c)
        {
            return;
        }

        FMOD_VECTOR pos = ToVec(src3d.Position);
        FMOD_VECTOR vel = ToVec(src3d.Velocity);
        c->set3DAttributes(&pos, &vel);
    }

    void FmodSoundEngine::SetListener(uint32_t index, const Listener3D& l)
    {
        if (!m_System)
        {
            return;
        }

        FMOD_VECTOR pos = ToVec(l.Position);
        FMOD_VECTOR vel = ToVec(l.Velocity);
        FMOD_VECTOR fwd = ToVec(l.Forward);
        FMOD_VECTOR up = ToVec(l.Up);

        FMOD_CHECK(m_System->set3DListenerAttributes((int)index, &pos, &vel, &fwd, &up));
    }

    void FmodSoundEngine::Update()
    {
        if (m_System)
        {
            m_System->update();
        }
    }
}
