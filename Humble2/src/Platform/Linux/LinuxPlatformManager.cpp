#include "LinuxPlatformManager.h"

#include "Core/Context.h"

#include <iostream>
#include <filesystem>

#ifdef HBL2_PLATFORM_LINUX
    #include <cstdlib>
#endif

namespace HBL2
{
    static std::string GetAppDataDir()
    {
#ifdef HBL2_PLATFORM_LINUX
        std::filesystem::path logDirectory;

        // Get the XDG Data Home path.
        const char* xdgDataHome = std::getenv("XDG_DATA_HOME");

        if (xdgDataHome && xdgDataHome[0] != '\0')
        {
            // Use the explicitly defined XDG path.
            logDirectory = std::filesystem::path(xdgDataHome) / "Humble";
        }
        else
        {
            // Fallback: XDG spec dictates using ~/.local/share if the variable is missing.
            const char* homeDir = std::getenv("HOME");
            if (!homeDir)
            {
                std::cerr << "Failed to find HOME directory." << std::endl;
                return "";
            }
            logDirectory = std::filesystem::path(homeDir) / ".local" / "share" / "Humble";
        }

        // Create the directory structure.
        std::error_code ec;
        std::filesystem::create_directories(logDirectory, ec);
        
        if (ec)
        {
            std::cerr << "Failed to create Linux log directory: " << e.what() << std::endl;
            return "";
        }

        return logDirectory.string();
#else
        return "";
#endif
    }

    void LinuxPlatformManager::Initialize()
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

    void LinuxPlatformManager::Shutdown()
    {
        
    }
}
