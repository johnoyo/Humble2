#include "PlatformManager.h"

#include "Platform/Windows/WindowsPlatformManager.h"
#include "Platform/MacOS/MacOSPlatformManager.h"
#include "Platform/Linux/LinuxPlatformManager.h"

namespace HBL2
{
    PlatformManager* PlatformManager::Instance = nullptr;

    PlatformManager* PlatformManager::Create()
    {
#ifdef HBL2_PLATFORM_WINDOWS
        return new WindowsPlatformManager;
#elif HBL2_PLATFORM_MACOS
        return new MacOSPlatformManager;
#elif HBL2_PLATFORM_LINUX
        return new LinuxPlatformManager;
#endif
    }

    const std::string& PlatformManager::GetAppDataDirectory()
    {
        return m_AppDataDirectory;
    }

    const std::string& PlatformManager::GetResourcesDirectory()
    {
        return m_ResourcesDirectory;
    }

    const std::string& PlatformManager::GetExecutableDirectory()
    {
        return m_ExecutableDirectory;
    }

    const BitFlags<CompressionMethod> PlatformManager::GetSupportedCompressionMethods(Platform platform) const
    {
        BitFlags<CompressionMethod> supportedCompressionMethods;

        switch (platform)
        {
        case Platform::Windows:
            supportedCompressionMethods.Set(CompressionMethod::NONE);
            supportedCompressionMethods.Set(CompressionMethod::BASISU);
            break;
        case Platform::MacOS:
            supportedCompressionMethods.Set(CompressionMethod::NONE);
            supportedCompressionMethods.Set(CompressionMethod::BASISU);
            supportedCompressionMethods.Set(CompressionMethod::ASTC);
            break;
        case Platform::Linux:
            supportedCompressionMethods.Set(CompressionMethod::NONE);
            supportedCompressionMethods.Set(CompressionMethod::BASISU);
            break;
        }
                
        return supportedCompressionMethods;
    }

    const BitFlags<Format> PlatformManager::GetSupportedTranscodingFormats(Platform platform) const
    {
        BitFlags<Format> supportedTranscodingFormats;

        switch (platform)
        {
        case Platform::Windows:
            supportedTranscodingFormats.Set(Format::BC1_RGBA_SRGB);
            supportedTranscodingFormats.Set(Format::BC1_RGBA_UNORM);
            supportedTranscodingFormats.Set(Format::BC1_RGB_SRGB);
            supportedTranscodingFormats.Set(Format::BC1_RGB_UNORM);
            supportedTranscodingFormats.Set(Format::BC3_SRGB);
            supportedTranscodingFormats.Set(Format::BC3_UNORM);
            supportedTranscodingFormats.Set(Format::BC7_SRGB);
            supportedTranscodingFormats.Set(Format::BC7_SRGB);
            supportedTranscodingFormats.Set(Format::BC7_UNORM);
            supportedTranscodingFormats.Set(Format::BC6H_UF);
            break;
        case Platform::MacOS:
            supportedTranscodingFormats.Set(Format::BC1_RGBA_SRGB);
            supportedTranscodingFormats.Set(Format::BC1_RGBA_UNORM);
            supportedTranscodingFormats.Set(Format::BC1_RGB_SRGB);
            supportedTranscodingFormats.Set(Format::BC1_RGB_UNORM);
            supportedTranscodingFormats.Set(Format::BC3_SRGB);
            supportedTranscodingFormats.Set(Format::BC3_UNORM);
            supportedTranscodingFormats.Set(Format::BC7_SRGB);
            supportedTranscodingFormats.Set(Format::BC7_SRGB);
            supportedTranscodingFormats.Set(Format::BC7_UNORM);
            supportedTranscodingFormats.Set(Format::BC6H_UF);
            supportedTranscodingFormats.Set(Format::ASTC_4x4_SRGB);
            supportedTranscodingFormats.Set(Format::ASTC_4x4_UNORM);
            break;
        case Platform::Linux:
            supportedTranscodingFormats.Set(Format::BC1_RGBA_SRGB);
            supportedTranscodingFormats.Set(Format::BC1_RGBA_UNORM);
            supportedTranscodingFormats.Set(Format::BC1_RGB_SRGB);
            supportedTranscodingFormats.Set(Format::BC1_RGB_UNORM);
            supportedTranscodingFormats.Set(Format::BC3_SRGB);
            supportedTranscodingFormats.Set(Format::BC3_UNORM);
            supportedTranscodingFormats.Set(Format::BC7_SRGB);
            supportedTranscodingFormats.Set(Format::BC7_SRGB);
            supportedTranscodingFormats.Set(Format::BC7_UNORM);
            supportedTranscodingFormats.Set(Format::BC6H_UF);
            break;
        }
                
        return supportedTranscodingFormats;
    }

    const BitFlags<Format> PlatformManager::GetSupportedCompressionFormats(Platform platform) const
    {
        BitFlags<Format> supportedCompressionFormats;

        switch (platform)
        {
        case Platform::Windows:
            supportedCompressionFormats.Set(Format::BC1_RGBA_SRGB);
            supportedCompressionFormats.Set(Format::BC1_RGBA_UNORM);
            supportedCompressionFormats.Set(Format::BC1_RGB_SRGB);
            supportedCompressionFormats.Set(Format::BC1_RGB_UNORM);
            supportedCompressionFormats.Set(Format::BC3_SRGB);
            supportedCompressionFormats.Set(Format::BC3_UNORM);
            supportedCompressionFormats.Set(Format::BC7_SRGB);
            supportedCompressionFormats.Set(Format::BC7_SRGB);
            supportedCompressionFormats.Set(Format::BC7_UNORM);
            supportedCompressionFormats.Set(Format::BC6H_UF);
            break;
        case Platform::MacOS:
            supportedCompressionFormats.Set(Format::BC1_RGBA_SRGB);
            supportedCompressionFormats.Set(Format::BC1_RGBA_UNORM);
            supportedCompressionFormats.Set(Format::BC1_RGB_SRGB);
            supportedCompressionFormats.Set(Format::BC1_RGB_UNORM);
            supportedCompressionFormats.Set(Format::BC3_SRGB);
            supportedCompressionFormats.Set(Format::BC3_UNORM);
            supportedCompressionFormats.Set(Format::BC7_SRGB);
            supportedCompressionFormats.Set(Format::BC7_SRGB);
            supportedCompressionFormats.Set(Format::BC7_UNORM);
            supportedCompressionFormats.Set(Format::BC6H_UF);
            supportedCompressionFormats.Set(Format::ASTC_4x4_SRGB);
            supportedCompressionFormats.Set(Format::ASTC_4x4_UNORM);
            supportedCompressionFormats.Set(Format::ASTC_5x5_SRGB);
            supportedCompressionFormats.Set(Format::ASTC_5x5_UNORM);
            supportedCompressionFormats.Set(Format::ASTC_6x6_SRGB);
            supportedCompressionFormats.Set(Format::ASTC_6x6_UNORM);
            supportedCompressionFormats.Set(Format::ASTC_8x8_SRGB);
            supportedCompressionFormats.Set(Format::ASTC_8x8_UNORM);
            supportedCompressionFormats.Set(Format::ASTC_10x10_SRGB);
            supportedCompressionFormats.Set(Format::ASTC_10x10_UNORM);
            supportedCompressionFormats.Set(Format::ASTC_12x12_SRGB);
            supportedCompressionFormats.Set(Format::ASTC_12x12_UNORM);
            break;
        case Platform::Linux:
            supportedCompressionFormats.Set(Format::BC1_RGBA_SRGB);
            supportedCompressionFormats.Set(Format::BC1_RGBA_UNORM);
            supportedCompressionFormats.Set(Format::BC1_RGB_SRGB);
            supportedCompressionFormats.Set(Format::BC1_RGB_UNORM);
            supportedCompressionFormats.Set(Format::BC3_SRGB);
            supportedCompressionFormats.Set(Format::BC3_UNORM);
            supportedCompressionFormats.Set(Format::BC7_SRGB);
            supportedCompressionFormats.Set(Format::BC7_SRGB);
            supportedCompressionFormats.Set(Format::BC7_UNORM);
            supportedCompressionFormats.Set(Format::BC6H_UF);
            break;
        }

        return supportedCompressionFormats;
    }

    Platform PlatformManager::GetPlatform()
    {
        return m_Platform;
    }
}
