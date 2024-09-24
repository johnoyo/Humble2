#pragma once

#include "Base.h"
#include "Scene\ISystem.h"

#include "Utilities\Random.h"

#include <windows.h>
#include <iostream>
#include <filesystem>

namespace HBL2
{
	class NativeScriptUtilities
	{
	public:
		NativeScriptUtilities(const NativeScriptUtilities&) = delete;

		static NativeScriptUtilities& Get()
		{
			HBL2_CORE_ASSERT(s_Instance != nullptr, "MeshUtilities::s_Instance is null! Call MeshUtilities::Initialize before use.");
			return *s_Instance;
		}

		static void Initialize();
		static void Shutdown();

		ISystem* LoadDLL(const std::string& dllPath);
		void DeleteDLLInstance(const std::string& dllName);

		std::string GetDefaultSystemCode(const std::string& systemName);		
		std::string GetDefaultSolutionText(const std::string& systemName);	
		std::string GetDefaultProjectText(const std::string& systemName);	

	private:
		NativeScriptUtilities() = default;

		std::unordered_map<std::string, HINSTANCE> m_DLLInstances;

		inline static NativeScriptUtilities* s_Instance = nullptr;
	};
}