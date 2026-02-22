#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"
#include "Resources\ResourceManager.h"

#include <Sound\SoundEngine.h>

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

	private:
		struct ChannelEntry
		{
			SoundEngine::ChannelHandle channel = SoundEngine::InvalidChannel;
			Entity owner = UINT32_MAX;
		};

		uint32_t AllocateSlot();
		void FreeSlot(uint32_t idx);

		void StartChannel(Component::AudioSource& src, Entity entity, bool paused);
		void StopChannel(ChannelEntry& e, uint32_t slotIndex);
		void PauseChannel(ChannelEntry& e, bool paused);
		void UpdateChannelParams(const Component::AudioSource& src, ChannelEntry& e);
		void Update3D(ChannelEntry& e, const Component::Transform& tr);

		void UpdateListener();

	private:
		bool m_Initialized = false;

		std::vector<ChannelEntry> m_Channels;
		std::queue<uint32_t>      m_Free;
	};
}