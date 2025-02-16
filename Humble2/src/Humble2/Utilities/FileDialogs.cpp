#include "FileDialogs.h"

#include "portable-file-dialogs.h"

namespace HBL2
{
	std::string FileDialogs::OpenFile(const std::string& title, const std::string& defaultPath, const std::vector<std::string>& filters)
	{
		const auto& result = pfd::open_file(title, defaultPath, filters).result();
		return result.size() == 0 ? "" : result[0];
	}

	std::string FileDialogs::SaveFile(const std::string& title, const std::string& defaultPath, const std::vector<std::string>& filters)
	{
		return pfd::save_file(title, defaultPath, filters).result();
	}

	void HBL2::FileUtils::CopyFolder(const std::filesystem::path& source, const std::filesystem::path& destination)
    {
        try
        {
            // Ensure source exists and is a directory
            if (!std::filesystem::exists(source) || !std::filesystem::is_directory(source))
            {
                HBL2_CORE_ERROR("Source folder {} does not exist or is not a directory.", source);
            }

            // Create destination if it doesn't exist
            if (!std::filesystem::exists(destination))
            {
                std::filesystem::create_directories(destination);
            }

            // Use std::filesystem::copy with recursive option
            std::filesystem::copy(source, destination, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

            HBL2_CORE_TRACE("Folder copied successfully from {} to {}",source, destination);
        }
        catch (const std::exception& e)
        {
            HBL2_CORE_ERROR("CopyFolder error: {}", e.what());
        }
    }
}

