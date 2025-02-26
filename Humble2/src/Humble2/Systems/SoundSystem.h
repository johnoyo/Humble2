#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"
#include "Resources\ResourceManager.h"

#include <fmod.hpp>
#include <fmod_errors.h>

namespace HBL2
{
	class HBL2_API SoundSystem final : public ISystem
	{
	public:
		SoundSystem() { Name = "SoundSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnDestroy() override;

	private:
		bool SucceededOrWarn(const std::string& message, FMOD_RESULT result);

	private:
		FMOD::System* m_SoundSystem = nullptr;
		FMOD::ChannelGroup* m_ChannelGroup = nullptr;
	};
}