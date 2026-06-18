#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "dusk/modding/mod_manifest.hpp"
#include "dusk/modding/shared_library.hpp"

namespace dusk::modding {

struct LoadedMod {
    ModManifest manifest;
    std::filesystem::path root;
    std::filesystem::path library_path;
    SharedLibrary library;
    bool initialized = false;
    std::string status;
};

bool load_mod_manifest(const std::filesystem::path& manifestPath, std::string* error);
bool load_mod_library(const std::filesystem::path& libraryPath, std::string* error);
std::span<const LoadedMod> loaded_mods();
std::string last_error();
void shutdown_mods();

}  // namespace dusk::modding

