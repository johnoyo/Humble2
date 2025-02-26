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

    std::string FileUtils::RelativePath(const std::filesystem::path& path, const std::filesystem::path& base)
    {
        const std::string& pathString = path.string();
        const std::string& baseString = base.string();

        size_t baseLen = baseString.size();
        size_t pathLen = pathString.size();

        // Check if 'path' starts with 'base'
        if (pathLen >= baseLen && std::strncmp(pathString.data(), baseString.data(), baseLen) == 0)
        {
            size_t start = baseLen;
            if (start < pathLen && (pathString[start] == '/' || pathString[start] == '\\'))
            {
                start++; // Skip the leading separator
            }

            return pathString.substr(start); // Only one allocation now
        }

        return pathString; // Only one allocation in worst case
    }
}

