#pragma once

#include <Base.h>

#include <fmod.hpp>
#include <fmod_errors.h>

#include <string>
#include <unordered_map>

namespace HBL2 
{
	class HBL2_API SoundManager 
	{
	public:
		SoundManager(const SoundManager&) = delete;

		static SoundManager& Get() 
		{
			HBL2_CORE_ASSERT(s_Instance != nullptr, "SoundManager::s_Instance is null! Call SoundManager::Initialize before use.");
			return *s_Instance;
		}

		static void Initialize();
		static void Shutdown();

		void Update();
		void Play(const std::string& source, bool playLooped = false, bool startPaused = false);
		void Stop(const std::string& source);

	private:
		SoundManager() {}

		void Start();
		bool Exists(const std::string& soundName);
		bool ChannelExists(const std::string& soundName);
		bool SucceededOrWarn(const std::string& message, FMOD_RESULT result);

		FMOD::System* m_SoundSystem = nullptr;
		FMOD::ChannelGroup* m_ChannelGroup = nullptr;
		FMOD_RESULT m_Result = FMOD_OK;
		std::unordered_map<std::string, FMOD::Sound*> m_Sounds;
		std::unordered_map<std::string, FMOD::Channel*> m_Channels;

		static SoundManager* s_Instance;
	};
}
