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
}