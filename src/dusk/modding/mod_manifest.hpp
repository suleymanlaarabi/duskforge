#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace dusk::modding {

struct ModManifest {
    std::string id;
    std::string name;
    std::string version;
    std::string sdk_version;
    std::string library;
    bool synthetic = false;
};

std::filesystem::path mods_directory();
std::optional<ModManifest> load_manifest(const std::filesystem::path& path, std::string* error);
std::filesystem::path resolve_library_path(
    const ModManifest& manifest, const std::filesystem::path& modRoot);
std::string platform_library_extension();
bool is_compatible_sdk_version(const std::string& version);

}  // namespace dusk::modding

