#pragma once

#include "SoundEngine.h"

#include <fmod.hpp>
#include <fmod_errors.h>

namespace HBL2
{
    class HBL2_API FmodSoundEngine final : public SoundEngine
    {
    public:
        ~FmodSoundEngine() override = default;

        virtual bool Initialize(void* engineContext, const SoundEngineConfig& cfg) override;
        virtual void Update() override;
        virtual void Shutdown() override;

        virtual SoundID LoadSound(const std::filesystem::path& absolutePath, const char* debugName, SoundLoadFlags flags) override;
        virtual void ReleaseSound(SoundID id) override;

        virtual ChannelHandle PlaySound(SoundID id, bool startPaused) override;
        virtual void StopChannel(ChannelHandle ch) override;
        virtual void SetPaused(ChannelHandle ch, bool paused) override;
        virtual bool IsValid(ChannelHandle ch) const override;

        virtual void SetVolume(ChannelHandle ch, float volume01) override;
        virtual void SetPitch(ChannelHandle ch, float pitch) override;
        virtual void SetLooping(ChannelHandle ch, bool looping) override;

        virtual void Set3DEnabled(ChannelHandle ch, bool enabled) override;
        virtual void Set3DAttributes(ChannelHandle ch, const Source3D& src3d) override;

        virtual void SetListener(uint32_t index, const Listener3D& l) override;

    private:
        static SoundID NewID();

        static FMOD::Channel* FromHandle(ChannelHandle h)
        {
            return reinterpret_cast<FMOD::Channel*>(static_cast<uintptr_t>(h));
        }

        static ChannelHandle ToHandle(FMOD::Channel* ch)
        {
            return static_cast<ChannelHandle>(reinterpret_cast<uintptr_t>(ch));
        }

        static FMOD_VECTOR ToVec(const glm::vec3& v)
        {
            return FMOD_VECTOR{ v.x, v.y, v.z };
        }

    private:
        FMOD::System* m_System = nullptr;
        FMOD::ChannelGroup* m_Group = nullptr;

        std::unordered_map<SoundID, FMOD::Sound*> m_Sounds;
    };
}