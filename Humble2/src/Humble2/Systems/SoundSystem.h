#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"
#include "Resources\ResourceManager.h"

#include <fmod.hpp>
#include <fmod_errors.h>

#include <queue>

namespace HBL2
{
	class HBL2_API SoundSystem final : public ISystem
	{
	public:
		static SoundSystem* Instance;

		SoundSystem();

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnDestroy() override;

		static void Play(Component::AudioSource& audioSource);
		static void Pause(Component::AudioSource& audioSource);
		static void Resume(Component::AudioSource& audioSource);
		static void Stop(Component::AudioSource& audioSource);

	private:
		struct ChannelEntry
		{
			FMOD::Channel* channel = nullptr;
			Entity owner = UINT32_MAX;
		};

		uint32_t AllocateSlot();
		void FreeSlot(uint32_t idx);

		void StartChannel(Component::AudioSource& src, uint32_t entity, bool paused);
		void StopChannel(ChannelEntry& e, uint32_t slotIndex);
		void PauseChannel(ChannelEntry& e, bool paused);
		void UpdateChannelParams(const Component::AudioSource& src, ChannelEntry& e);
		void Update3D(ChannelEntry& e, const Component::Transform& tr);

		void UpdateListener();

	private:
		FMOD::System* m_SoundSystem = nullptr;
		FMOD::ChannelGroup* m_ChannelGroup = nullptr;

		std::vector<ChannelEntry> m_Channels;
		std::queue<uint32_t>      m_Free;
	};
}