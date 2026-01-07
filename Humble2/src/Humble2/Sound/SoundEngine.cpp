#include "SoundEngine.h"

#include "Resources\ResourceManager.h"

namespace HBL2
{
    SoundEngine* SoundEngine::Instance = nullptr;

    void SoundEngine::Play(Component::AudioSource& audioSource)
    {
        audioSource.State = Component::AudioSource::PlaybackState::Trigger;
    }

    void SoundEngine::Pause(Component::AudioSource& audioSource)
    {
        audioSource.State = Component::AudioSource::PlaybackState::Paused;
    }

    void SoundEngine::Resume(Component::AudioSource& audioSource)
    {
        audioSource.State = Component::AudioSource::PlaybackState::Resume;
    }

    void SoundEngine::Stop(Component::AudioSource& audioSource)
    {
        audioSource.State = Component::AudioSource::PlaybackState::Stopped;
    }

    void SoundEngine::Play(Scene* ctx, Entity e)
    {
        if (ctx == nullptr)
        {
            HBL2_CORE_ERROR("Cannot play sound, the provided scene is null.");
            return;
        }

        Component::AudioSource* audioSource = ctx->TryGetComponent<Component::AudioSource>(e);
        if (audioSource != nullptr)
        {
            Play(*audioSource);
            return;
        }

        HBL2_CORE_ERROR("Cannot play sound, the provided entity does not have an Component::AudioSource.");
        return;
    }

    void SoundEngine::Pause(Scene* ctx, Entity e)
    {
        if (ctx == nullptr)
        {
            HBL2_CORE_ERROR("Cannot pause sound, the provided scene is null.");
            return;
        }

        Component::AudioSource* audioSource = ctx->TryGetComponent<Component::AudioSource>(e);
        if (audioSource != nullptr)
        {
            Pause(*audioSource);
            return;
        }

        HBL2_CORE_ERROR("Cannot pause sound, the provided entity does not have an Component::AudioSource.");
        return;
    }

    void SoundEngine::Resume(Scene* ctx, Entity e)
    {
        if (ctx == nullptr)
        {
            HBL2_CORE_ERROR("Cannot resume sound, the provided scene is null.");
            return;
        }

        Component::AudioSource* audioSource = ctx->TryGetComponent<Component::AudioSource>(e);
        if (audioSource != nullptr)
        {
            Resume(*audioSource);
            return;
        }

        HBL2_CORE_ERROR("Cannot resume sound, the provided entity does not have an Component::AudioSource.");
        return;
    }

    void SoundEngine::Stop(Scene* ctx, Entity e)
    {
        if (ctx == nullptr)
        {
            HBL2_CORE_ERROR("Cannot stop sound, the provided scene is null.");
            return;
        }

        Component::AudioSource* audioSource = ctx->TryGetComponent<Component::AudioSource>(e);
        if (audioSource != nullptr)
        {
            Stop(*audioSource);
            return;
        }

        HBL2_CORE_ERROR("Cannot stop sound, the provided entity does not have an Component::AudioSource.");
        return;
    }
}
