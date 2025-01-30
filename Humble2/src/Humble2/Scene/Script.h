#pragma once

#include "Humble2API.h"

#include <filesystem>

namespace HBL2
{
	enum class HBL2_API ScriptType
	{
		NONE = 0,
		UNITY_BUILD_SCRIPT,
		SYSTEM,
		COMPONENT,
		EDITOR,
		RENDER_PASS,
	};

	struct HBL2_API ScriptDescriptor
	{
		const char* debugName;
		ScriptType type = ScriptType::NONE;
		std::filesystem::path path;
	};

	struct HBL2_API Script
	{
		Script() = default;
		Script(const ScriptDescriptor&& desc)
		{
			Name = desc.debugName;
			Type = desc.type;
			Path = desc.path;
		}

		std::string Name;
		ScriptType Type = ScriptType::NONE;
		std::filesystem::path Path;
	};
}