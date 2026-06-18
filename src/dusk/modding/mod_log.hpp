#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dusk::modding {

enum class ModLogLevel {
    Debug = 0,
    Info,
    Warning,
    Error,
};

struct ModLogEntry {
    uint64_t sequence = 0;
    ModLogLevel level = ModLogLevel::Info;
    std::string mod_id;
    std::string message;
};

void append_mod_log(ModLogLevel level, std::string modId, std::string message);
std::vector<ModLogEntry> mod_log_entries();
uint64_t mod_log_sequence();
void clear_mod_log();
const char* mod_log_level_name(ModLogLevel level);

}  // namespace dusk::modding
