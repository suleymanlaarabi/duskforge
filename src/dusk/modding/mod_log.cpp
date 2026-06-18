#include "dusk/modding/mod_log.hpp"

#include <deque>
#include <mutex>
#include <utility>

#include "dusk/logging.h"
#include "dusklight/modding/log.hpp"

namespace dusk::modding {
namespace {

constexpr size_t kMaxModLogEntries = 512;

std::mutex gModLogMutex;
std::deque<ModLogEntry> gModLogEntries;
uint64_t gModLogSequence = 0;

void write_to_engine_log(ModLogLevel level, const std::string& modId, const std::string& message) {
    switch (level) {
    case ModLogLevel::Debug:
        DuskLog.debug("[mod:{}] {}", modId, message);
        break;
    case ModLogLevel::Info:
        DuskLog.info("[mod:{}] {}", modId, message);
        break;
    case ModLogLevel::Warning:
        DuskLog.warn("[mod:{}] {}", modId, message);
        break;
    case ModLogLevel::Error:
        DuskLog.error("[mod:{}] {}", modId, message);
        break;
    }
}

}  // namespace

void append_mod_log(ModLogLevel level, std::string modId, std::string message) {
    if (modId.empty()) {
        modId = "unknown";
    }

    write_to_engine_log(level, modId, message);

    std::lock_guard lock(gModLogMutex);
    ++gModLogSequence;
    gModLogEntries.push_back({
        .sequence = gModLogSequence,
        .level = level,
        .mod_id = std::move(modId),
        .message = std::move(message),
    });
    while (gModLogEntries.size() > kMaxModLogEntries) {
        gModLogEntries.pop_front();
    }
}

std::vector<ModLogEntry> mod_log_entries() {
    std::lock_guard lock(gModLogMutex);
    return {gModLogEntries.begin(), gModLogEntries.end()};
}

uint64_t mod_log_sequence() {
    std::lock_guard lock(gModLogMutex);
    return gModLogSequence;
}

void clear_mod_log() {
    std::lock_guard lock(gModLogMutex);
    gModLogEntries.clear();
    ++gModLogSequence;
}

const char* mod_log_level_name(ModLogLevel level) {
    switch (level) {
    case ModLogLevel::Debug:
        return "debug";
    case ModLogLevel::Info:
        return "info";
    case ModLogLevel::Warning:
        return "warn";
    case ModLogLevel::Error:
        return "error";
    }
    return "info";
}

}  // namespace dusk::modding

extern "C" DUSK_MOD_LOG_EXPORT void dusk_mod_log_write(
    int level, const char* mod_id, const char* message) {
    auto logLevel = dusk::modding::ModLogLevel::Info;
    switch (level) {
    case static_cast<int>(dusk::modding::ModLogLevel::Debug):
        logLevel = dusk::modding::ModLogLevel::Debug;
        break;
    case static_cast<int>(dusk::modding::ModLogLevel::Warning):
        logLevel = dusk::modding::ModLogLevel::Warning;
        break;
    case static_cast<int>(dusk::modding::ModLogLevel::Error):
        logLevel = dusk::modding::ModLogLevel::Error;
        break;
    default:
        break;
    }
    dusk::modding::append_mod_log(logLevel, mod_id != nullptr ? mod_id : "unknown",
        message != nullptr ? message : "");
}
