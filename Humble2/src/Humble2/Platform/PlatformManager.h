#pragma once

#include "Humble2API.h"

#include "Renderer/Enums.h"
#include "Utilities/TextureUtilities.h"
#include "Utilities/Collections/BitFlags.h"

#include <string>

namespace HBL2
{
    enum class HBL2_API Platform
    {
        Windows,
        MacOS,
        Linux,
        Web,
        None
    };

    class HBL2_API PlatformManager
    {
    public:
        static PlatformManager* Instance;

        PlatformManager() = default;
        virtual ~PlatformManager() = default;
        
        static PlatformManager* Create();
        
        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;
        
        const std::string& GetAppDataDirectory();
        const std::string& GetExecutableDirectory();
        const std::string& GetResourcesDirectory();

        const BitFlags<CompressionMethod> GetSupportedCompressionMethods(Platform platform) const;
        const BitFlags<Format> GetSupportedTranscodingFormats(Platform platform) const;
        const BitFlags<Format> GetSupportedCompressionFormats(Platform platform) const;

        Platform GetPlatform();
        
    protected:
        std::string m_AppDataDirectory;
        std::string m_ExecutableDirectory;
        std::string m_ResourcesDirectory;
        std::string m_EmptyDirectory = "";

        Platform m_Platform;
    };
}
