#pragma once

#include "Sound.h"
#include "Humble2API.h"
#include "Scene\Scene.h"
#include "Scene\Components.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <filesystem>

namespace HBL2
{
    struct SoundEngineConfig
    {
        uint32_t MaxChannels = 512;
        bool Enable3D = true;
    };

    struct Listener3D
    {
        glm::vec3 Position{ 0.f };
        glm::vec3 Velocity{ 0.f };
        glm::vec3 Forward{ 0.f, 0.f, 1.f };
        glm::vec3 Up{ 0.f, 1.f, 0.f };
    };

    struct Source3D
    {
        glm::vec3 Position{ 0.f };
        glm::vec3 Velocity{ 0.f };
    };

    enum class SoundLoadFlags : uint32_t
    {
        None = 0,
        Stream = 1 << 0,
        Compressed = 1 << 1,
        NonBlocking = 1 << 2,
        Spatial3D = 1 << 3,
    };

    inline SoundLoadFlags operator|(SoundLoadFlags a, SoundLoadFlags b)
    {
        return (SoundLoadFlags)((uint32_t)a | (uint32_t)b);
    }
    inline bool HasFlag(SoundLoadFlags v, SoundLoadFlags f)
    {
        return ((uint32_t)v & (uint32_t)f) != 0;
    }

    class HBL2_API SoundEngine
    {
    public:
        using ChannelHandle = uint64_t;
        static constexpr ChannelHandle InvalidChannel = 0;

        static SoundEngine* Instance;

        virtual ~SoundEngine() = default;

        virtual bool Initialize(void* engineContext, const SoundEngineConfig& cfg) = 0;
        virtual void Update() = 0;
        virtual void Shutdown() = 0;

        // Asset lifetime.
        virtual SoundID LoadSound(const std::filesystem::path& absolutePath, const char* debugName, SoundLoadFlags flags) = 0;
        virtual void ReleaseSound(SoundID id) = 0;

        // Low-level playback (used by SoundSystem).
        virtual ChannelHandle PlaySound(SoundID id, bool startPaused) = 0;
        virtual void StopChannel(ChannelHandle ch) = 0;
        virtual void SetPaused(ChannelHandle ch, bool paused) = 0;
        virtual bool IsValid(ChannelHandle ch) const = 0;

        virtual void SetVolume(ChannelHandle ch, float volume01) = 0;
        virtual void SetPitch(ChannelHandle ch, float pitch) = 0;
        virtual void SetLooping(ChannelHandle ch, bool looping) = 0;

        virtual void Set3DEnabled(ChannelHandle ch, bool enabled) = 0;
        virtual void Set3DAttributes(ChannelHandle ch, const Source3D& src3d) = 0;

        virtual void SetListener(uint32_t index, const Listener3D& l) = 0;

        // User-facing convenience API.
        void Play(Component::AudioSource& audioSource);
        void Pause(Component::AudioSource& audioSource);
        void Resume(Component::AudioSource& audioSource);
        void Stop(Component::AudioSource& audioSource);

        void Play(Scene* ctx, Entity e);
        void Pause(Scene* ctx, Entity e);
        void Resume(Scene* ctx, Entity e);
        void Stop(Scene* ctx, Entity e);
    };
}