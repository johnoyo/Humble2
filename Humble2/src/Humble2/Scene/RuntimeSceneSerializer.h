#pragma once

#include "Scene.h"
#include "Asset/AssetManager.h"
#include "Serialization/FileStreamWriter.h"
#include "Serialization/FileStreamReader.h"

namespace HBL2
{
	class RuntimeSceneSerializer
	{
	public:
		RuntimeSceneSerializer(Scene* scene);

		void Serialize(FileStreamWriter& stream);
		bool Deserialize(FileStreamReader& stream);

	private:
		Scene* m_Scene = nullptr;
	};
}
