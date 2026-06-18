#include "dusk/modding/mod_manifest.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <string_view>

#include "dusk/data.hpp"
#include "dusk/io.hpp"
#include "dusklight/modding/version.hpp"
#include "nlohmann/json.hpp"

namespace dusk::modding {
namespace {

using json = nlohmann::json;

bool valid_id(std::string_view id) {
    if (id.empty()) {
        return false;
    }
    return std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return std::islower(c) || std::isdigit(c) || c == '_';
    });
}

bool valid_library_name(std::string_view name) {
    return !name.empty() && name.find('/') == std::string_view::npos &&
           name.find('\\') == std::string_view::npos &&
           name.find("..") == std::string_view::npos;
}

std::optional<int> parse_version_part(std::string_view& version) {
    const auto dot = version.find('.');
    const auto part = dot == std::string_view::npos ? version : version.substr(0, dot);
    int value = 0;
    const auto [ptr, ec] = std::from_chars(part.data(), part.data() + part.size(), value);
    if (ec != std::errc{} || ptr != part.data() + part.size()) {
        return std::nullopt;
    }
    version = dot == std::string_view::npos ? std::string_view{} : version.substr(dot + 1);
    return value;
}

std::optional<std::string> required_string(const json& value, const char* key, std::string* error) {
    const auto it = value.find(key);
    if (it == value.end() || !it->is_string()) {
        if (error) {
            *error = std::string{"missing string field: "} + key;
        }
        return std::nullopt;
    }
    return it->get<std::string>();
}

}  // namespace

std::filesystem::path mods_directory() {
    return data::configured_data_path() / "mods";
}

std::optional<ModManifest> load_manifest(const std::filesystem::path& path, std::string* error) {
    try {
        const auto bytes = io::FileStream::ReadAllBytes(path);
        const std::string text{reinterpret_cast<const char*>(bytes.data()), bytes.size()};
        const auto parsed = json::parse(text);
        if (!parsed.is_object()) {
            if (error) {
                *error = "mod.json must contain an object";
            }
            return std::nullopt;
        }

        ModManifest manifest;
        auto id = required_string(parsed, "id", error);
        auto name = required_string(parsed, "name", error);
        auto version = required_string(parsed, "version", error);
        auto sdkVersion = required_string(parsed, "sdk_version", error);
        auto library = required_string(parsed, "library", error);
        if (!id || !name || !version || !sdkVersion || !library) {
            return std::nullopt;
        }

        manifest.id = *id;
        manifest.name = *name;
        manifest.version = *version;
        manifest.sdk_version = *sdkVersion;
        manifest.library = *library;

        if (!valid_id(manifest.id)) {
            if (error) {
                *error = "id must match [a-z0-9_]+";
            }
            return std::nullopt;
        }
        if (!valid_library_name(manifest.library)) {
            if (error) {
                *error = "library must be a basename without path segments";
            }
            return std::nullopt;
        }
        if (!is_compatible_sdk_version(manifest.sdk_version)) {
            if (error) {
                *error = "sdk_version is not compatible with this Dusklight SDK";
            }
            return std::nullopt;
        }

        return manifest;
    } catch (const std::exception& e) {
        if (error) {
            *error = e.what();
        }
        return std::nullopt;
    }
}

std::filesystem::path resolve_library_path(
    const ModManifest& manifest, const std::filesystem::path& modRoot) {
    std::filesystem::path library = manifest.library;
    if (!library.has_extension()) {
        library += platform_library_extension();
    }
    return modRoot / library;
}

std::string platform_library_extension() {
#if defined(_WIN32)
    return ".dll";
#elif defined(__APPLE__)
    return ".dylib";
#else
    return ".so";
#endif
}

bool is_compatible_sdk_version(const std::string& version) {
    std::string_view rest{version};
    const auto major = parse_version_part(rest);
    const auto minor = parse_version_part(rest);
    if (!major || !minor) {
        return false;
    }
    return *major == DUSKLIGHT_SDK_VERSION_MAJOR && *minor <= DUSKLIGHT_SDK_VERSION_MINOR;
}

}  // namespace dusk::modding

