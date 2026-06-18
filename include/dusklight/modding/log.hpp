#pragma once

#include <string>
#include <string_view>

#if defined(_WIN32)
#define DUSK_MOD_LOG_EXPORT __declspec(dllexport)
#else
#define DUSK_MOD_LOG_EXPORT __attribute__((visibility("default")))
#endif

extern "C" DUSK_MOD_LOG_EXPORT void dusk_mod_log_write(
    int level, const char* mod_id, const char* message);

namespace dusklight::log {

enum class Level {
    Debug = 0,
    Info,
    Warning,
    Error,
};

inline void write(Level level, std::string_view mod_id, std::string_view message) {
    const std::string mod{mod_id};
    const std::string text{message};
    dusk_mod_log_write(static_cast<int>(level), mod.c_str(), text.c_str());
}

inline void debug(std::string_view mod_id, std::string_view message) {
    write(Level::Debug, mod_id, message);
}

inline void info(std::string_view mod_id, std::string_view message) {
    write(Level::Info, mod_id, message);
}

inline void warn(std::string_view mod_id, std::string_view message) {
    write(Level::Warning, mod_id, message);
}

inline void error(std::string_view mod_id, std::string_view message) {
    write(Level::Error, mod_id, message);
}

}  // namespace dusklight::log
