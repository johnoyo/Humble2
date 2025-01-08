#include "UserInterfaceUtilities.h"

namespace HBL2
{
	namespace UI
	{
		namespace Utils
		{
			ImVec2 GetViewportSize()
			{
				return *(ImVec2*)&Context::ViewportSize;
			}

			ImVec2 GetViewportPosition()
			{
				return *(ImVec2*)&Context::ViewportPosition;
			}

			float GetFontSize()
			{
				return ImGui::GetFontSize();
			}
		}
	}

	void RegisterComponentToReflection(const std::string& structCode)
	{
		// Regex pattern to match member declarations
		std::regex memberRegex(R"(([a-zA-Z_][a-zA-Z0-9_:<>]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)(?:\s*=\s*[^;]*)?;)");

		// Extract the struct name
		std::regex structNameRegex(R"(struct\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\{)");
		std::smatch match;

		std::string structName;
		if (std::regex_search(structCode, match, structNameRegex))
		{
			structName = match[1];
		}
		else
		{
			std::cerr << "Error: Could not extract struct name!" << std::endl;
			return;
		}

		// Begin generating reflection code
		std::ostringstream reflectionCode;
		reflectionCode << "entt::meta<" << structName << ">()\n";
		reflectionCode << "    .type(\"entt::hashed_string(typeid(" << structName << "\").name()))";

		// Match and process each member
		auto memberBegin = std::sregex_iterator(structCode.begin(), structCode.end(), memberRegex);
		auto memberEnd = std::sregex_iterator();

		for (auto it = memberBegin; it != memberEnd; ++it)
		{
			std::string type = (*it)[1].str();
			std::string name = (*it)[2].str();

			reflectionCode << "\n    .data<&" << structName << "::" << name << ">(\"" << name << "\"_hs).prop(\"name\"_hs, \"" << name << "\")";
		}

		reflectionCode << ";\n";

		// Output the generated code
		std::cout << "Generated Reflection Code:\n" << reflectionCode.str() << std::endl;
	}

	EditorUtilities& EditorUtilities::Get()
	{
		static EditorUtilities instance;
		return instance;
	}
}
