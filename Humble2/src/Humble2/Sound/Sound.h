#pragma once

#include "Humble2API.h"

#include <fmod.hpp>
#include <fmod_errors.h>

#include <string>
#include <filesystem>

namespace HBL2
{
	struct HBL2_API SoundDescriptor
	{
		const char* debugName = "";
		std::filesystem::path path;
		bool loop = false;
		bool startPaused = false;
	};

	struct HBL2_API Sound
	{
		Sound() = default;
		Sound(const SoundDescriptor&& desc)
		{
			Name = desc.debugName;
			Path = desc.path;
			Loop = desc.loop;
			StartPaused = desc.startPaused;

			Instance = nullptr;
			Channel = nullptr;
		}

		void Create(FMOD::System* soundSystem);
		void Play(FMOD::System* soundSystem);
		void SetPaused(bool paused);
		void Stop();
		void Destroy();

		std::string Name;
		std::filesystem::path Path;
		bool Loop = false;
		bool StartPaused = false;
		FMOD::Sound* Instance = nullptr;
		FMOD::Channel* Channel = nullptr;
	};
}