#include "dusk/modding/mod_manager.hpp"

#include <algorithm>
#include <cctype>
#include <system_error>

#include "dusk/io.hpp"
#include "dusk/logging.h"
#include "dusk/modding/sdk_world.hpp"
#include "dusklight/sdk.hpp"

namespace dusk::modding {
namespace {

std::vector<LoadedMod> gLoadedMods;
std::string gLastError;

std::string make_synthetic_id(const std::filesystem::path& path) {
    std::string id = path.stem().string();
    for (char& c : id) {
        const auto uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc)) {
            c = static_cast<char>(std::tolower(uc));
        } else {
            c = '_';
        }
    }
    if (id.empty()) {
        id = "mod";
    }
    return id;
}

bool set_error(std::string message, std::string* error) {
    gLastError = std::move(message);
    if (error) {
        *error = gLastError;
    }
    DuskLog.warn("Modding: {}", gLastError);
    return false;
}

bool already_loaded(std::string_view id) {
    return std::any_of(gLoadedMods.begin(), gLoadedMods.end(), [id](const LoadedMod& mod) {
        return mod.manifest.id == id;
    });
}

bool load_resolved_mod(LoadedMod mod, std::string* error) {
    if (already_loaded(mod.manifest.id)) {
        return set_error("mod already loaded: " + mod.manifest.id, error);
    }

    std::error_code ec;
    if (!std::filesystem::exists(mod.library_path, ec)) {
        return set_error("mod library does not exist: " + io::fs_path_to_string(mod.library_path),
            error);
    }

    std::string loadError;
    if (!mod.library.open(mod.library_path, &loadError)) {
        return set_error("failed to open mod library: " + loadError, error);
    }

    std::string symbolError;
    auto* symbol = mod.library.symbol("dusk_mod_init", &symbolError);
    if (!symbol) {
        return set_error("missing dusk_mod_init: " + symbolError, error);
    }

    initialize();
    auto init = reinterpret_cast<DuskModInitFn>(symbol);
    try {
        init(&world());
    } catch (const std::exception& e) {
        return set_error("dusk_mod_init threw: " + std::string{e.what()}, error);
    } catch (...) {
        return set_error("dusk_mod_init threw an unknown exception", error);
    }

    mod.initialized = true;
    mod.status = mod.manifest.synthetic ? "Loaded without manifest" : "Loaded";
    DuskLog.info("Loaded mod '{}' from {}", mod.manifest.id,
        io::fs_path_to_string(mod.library_path));
    gLoadedMods.emplace_back(std::move(mod));
    gLastError.clear();
    return true;
}

}  // namespace

bool load_mod_manifest(const std::filesystem::path& manifestPath, std::string* error) {
    std::string parseError;
    auto manifest = load_manifest(manifestPath, &parseError);
    if (!manifest) {
        return set_error("invalid mod manifest: " + parseError, error);
    }

    LoadedMod mod;
    mod.manifest = std::move(*manifest);
    mod.root = manifestPath.parent_path();
    mod.library_path = resolve_library_path(mod.manifest, mod.root);
    return load_resolved_mod(std::move(mod), error);
}

bool load_mod_library(const std::filesystem::path& libraryPath, std::string* error) {
    if (libraryPath.filename() == "mod.json") {
        return load_mod_manifest(libraryPath, error);
    }

    LoadedMod mod;
    mod.root = libraryPath.parent_path();
    mod.library_path = libraryPath;
    mod.manifest.id = make_synthetic_id(libraryPath);
    mod.manifest.name = libraryPath.stem().string();
    mod.manifest.version = "0.0.0";
    mod.manifest.sdk_version = DUSKLIGHT_SDK_VERSION;
    mod.manifest.library = libraryPath.filename().string();
    mod.manifest.synthetic = true;
    return load_resolved_mod(std::move(mod), error);
}

std::span<const LoadedMod> loaded_mods() {
    return gLoadedMods;
}

std::string last_error() {
    return gLastError;
}

void shutdown_mods() {
    gLoadedMods.clear();
    gLastError.clear();
}

}  // namespace dusk::modding

