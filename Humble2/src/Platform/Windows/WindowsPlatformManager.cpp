#include "WindowsPlatformManager.h"

#include "Core/Context.h"

#include <iostream>
#include <filesystem>

#ifdef HBL2_PLATFORM_WINDOWS
    #include <shlobj.h>
#endif

namespace HBL2
{
    static std::string GetAppDataDir()
    {
#ifdef HBL2_PLATFORM_WINDOWS
        PWSTR pszPath = NULL;
        std::wstring localAppDataPath = L"";

        // Fetch the Local AppData folder using the modern Windows API.
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pszPath)))
        {
            // Convert the wide-character pointer directly to a std::wstring.
            localAppDataPath = pszPath;
            
            // Free the memory allocated by the Windows shell.
            CoTaskMemFree(pszPath);
        }
        else
        {
            std::wcerr << L"Failed to retrieve Local AppData path." << std::endl;
            return "";
        }

        // Use C++17 filesystem to safely combine paths with correct backslashes.
        auto logDirectory = std::filesystem::path(localAppDataPath) / L"Humble";

        // Create the full folder structure if it does not exist yet.
        std::error_code ec;
        std::filesystem::create_directories(logDirectory, ec);
        
        if (ec)
        {
            std::cerr << "Failed to create directory: " << ec.message() << std::endl;
            return "";
        }

        return logDirectory.string();
#else
        return "";
#endif
    }

    void WindowsPlatformManager::Initialize()
    {
        if (Context::Mode == Mode::Editor)
        {
            m_AppDataDirectory = GetAppDataDir() + "/Editor";
        }
        else
        {
            m_AppDataDirectory = GetAppDataDir() + "/Runtime";
        }
        
        m_ResourcesDirectory = std::filesystem::current_path().string();
        m_ExecutableDirectory = std::filesystem::current_path().string();
    }

    void WindowsPlatformManager::Shutdown()
    {
        
    }
}
