#include "Sound.h"

#include <Project\Project.h>
#include <Utilities\Log.h>

namespace HBL2
{
	void Sound::Create(FMOD::System* soundSystem)
	{
		const std::string& finalPath = Project::GetAssetFileSystemPath(Path).string();

		FMOD_RESULT result = soundSystem->createSound(finalPath.c_str(), FMOD_DEFAULT, nullptr, &Instance);
		if (result != FMOD_OK)
		{
			HBL2_CORE_ERROR("[FMOD] Failed to create sound: ({}) {}", result, FMOD_ErrorString(result));
			return;
		}

		if (Loop)
		{
			Instance->setMode(FMOD_LOOP_NORMAL);
		}
		else
		{
			Instance->setMode(FMOD_LOOP_OFF);
		}
	}

	void Sound::Play(FMOD::System* soundSystem)
	{
		if (Channel == nullptr)
		{
			FMOD_RESULT result = soundSystem->playSound(Instance, nullptr, StartPaused, &Channel);

			if (result != FMOD_OK)
			{
				HBL2_CORE_ERROR("[FMOD] Failed to play sound: ({}) {}", result, FMOD_ErrorString(result));
			}
		}
	}

	void Sound::SetPaused(bool paused)
	{
		if (Channel == nullptr)
		{
			HBL2_CORE_WARN("[FMOD] Failed to set sound pause state, channel is null.");
			return;
		}

		FMOD_RESULT result = Channel->setPaused(paused);
		if (result != FMOD_OK)
		{
			HBL2_CORE_ERROR("[FMOD] Failed to stop sound: ({}) {}", result, FMOD_ErrorString(result));
		}
	}

	void Sound::Stop()
	{
		if (Channel == nullptr)
		{
			HBL2_CORE_WARN("[FMOD] Failed to stop sound, channel is null.");
			return;
		}

		FMOD_RESULT result = Channel->stop();
		if (result != FMOD_OK)
		{
			HBL2_CORE_ERROR("[FMOD] Failed to stop sound: ({}) {}", result, FMOD_ErrorString(result));
		}
		Channel = nullptr;
	}
	
	void Sound::Destroy()
	{
		Instance->release();
		Instance = nullptr;
		Channel = nullptr;
	}
}
