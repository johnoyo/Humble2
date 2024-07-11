#include "AssetManager.h"

namespace HBL2
{
    void AssetManager::Serialize(Asset* asset)
    {
        std::ofstream fout(asset->Path.string() + ".meta", 0);

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "fileFormatVersion" << YAML::Value << asset->FileFormatVersion;
        out << YAML::Key << "uuid" << YAML::Value << asset->UUID;
        out << YAML::Key << "userData" << YAML::Value << (uint64_t)asset->UserData;
        out << YAML::EndMap;

        fout << out.c_str();

        fout.close();
    }
}